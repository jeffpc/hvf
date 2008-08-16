/*
 * Copyright (c) 2007 Josef 'Jeff' Sipek
 */

#include <channel.h>
#include <io.h>
#include <interrupt.h>
#include <device.h>
#include <sched.h>
#include <atomic.h>
#include <spinlock.h>
#include <buddy.h>

static atomic_t numops;		/* # of I/O ops in-flight */
static LIST_HEAD(ops);		/* in-flight ops */
static spinlock_t ops_lock = SPIN_LOCK_UNLOCKED;

/*
 * Helper function to make sure the io_op has everything set right
 */
static int __verify_io_op(struct io_op *ioop)
{
	// FIXME: check everything that makes sense to check
	return 0;
}

static void __reset_reserved_fields(struct io_op *ioop)
{
	ioop->orb.__zero1 = 0;
	ioop->orb.__zero2 = 0;

	ioop->orb.__reserved1 = 0;
	ioop->orb.__reserved2 = 0;
	ioop->orb.__reserved3 = 0;
	ioop->orb.__reserved4 = 0;
	ioop->orb.__reserved5 = 0;
	ioop->orb.__reserved6 = 0;
}

/*
 * Submit an I/O request to a subchannel, and set up everything needed to
 * handle the operation
 */
int submit_io(struct io_op *ioop, int flags)
{
	static atomic_t op_id_counter;
	unsigned long intmask;

	int err = -EBUSY;

	if (!atomic_add_unless(&numops, 1, MAX_IOS))
		goto out;

	err = __verify_io_op(ioop);
	if (err)
		goto out_dec;

	/* make sure all reserved fields have the right values */
	__reset_reserved_fields(ioop);

	ioop->err = 0;
	atomic_set(&ioop->done, 0);

	/* add it to the list of ops */
	spin_lock_intsave(&ops_lock, &intmask);
	list_add_tail(&ioop->list, &ops);
	spin_unlock_intrestore(&ops_lock, intmask);

	ioop->orb.param = atomic_inc_return(&op_id_counter);

	/* Start the I/O */
	err = start_sch(ioop->ssid, &ioop->orb);
	if (!err)
		goto out;

	/* error while submitting I/O, let's unregister */

	spin_lock_intsave(&ops_lock, &intmask);
	list_del(&ioop->list);
	spin_unlock_intrestore(&ops_lock, intmask);

out_dec:
	atomic_dec(&numops);
out:
	return err;
}

/*
 * Initialize the channel I/O subsystem
 */
void init_io()
{
	u64 cr6;

	atomic_set(&numops, 0);

	/* enable all I/O interrupt classes */
	asm volatile(
		"stctg	6,6,%0\n"	/* get cr6 */
		"oi	%1,0xff\n"	/* enable all */
		"lctlg	6,6,%0\n"	/* reload cr6 */
	: /* output */
	: /* input */
	  "m" (cr6),
	  "m" (*(u64*) (((u8*)&cr6) + 4))
	);
}

static int default_io_handler(struct io_op *ioop, struct irb *irb)
{
	ioop->err = -EAGAIN;

	/* Unit check? */
	if (irb->status.dev_status & 0x02)
		ioop->err = -EUCHECK; /* FIXME: we should bail */

	/* Device End is set, we're done */
	if (irb->status.dev_status & 0x04)
		ioop->err = 0;

	return 0;
}

static void __cpu_initiated_io(struct io_op *ioop, struct irb *irb)
{
	unsigned long intmask;

	ioop->err = test_sch(ioop->ssid, irb);

	if (!ioop->err && ioop->handler)
		ioop->handler(ioop, irb);
	else if (!ioop->err)
		default_io_handler(ioop, irb);

	/*
	 * We can do this, because the test_sch function sets ->err, and
	 * therefore regardless of ->handler being defined, ->err will have
	 * a reasonable value
	 */
	if (ioop->err == -EAGAIN)
		return; /* leave handler registered */

	/* flag io_op as done... */
	atomic_set(&ioop->done, 1);

	/* ...and remove it form the list */
	spin_lock_intsave(&ops_lock, &intmask);
	list_del(&ioop->list);
	spin_unlock_intrestore(&ops_lock, intmask);

	atomic_dec(&numops);

	/* call the destructor if there is one */
	if (ioop->dtor)
		ioop->dtor(ioop);
}

static void __dev_initiated_io(struct device *dev, struct irb *irb)
{
}

/*
 * I/O Interrupt handler (C portion)
 */
void __io_int_handler()
{
	unsigned long intmask;
	struct io_op *ioop;
	struct device *dev;
	struct irb irb;

	/*
	 * Scan the ops list to see if it is a CPU-initiated operation
	 */
	spin_lock_intsave(&ops_lock, &intmask);
	list_for_each_entry(ioop, &ops, list) {
		if (ioop->orb.param == IO_INT_CODE->param &&
		    ioop->ssid == IO_INT_CODE->ssid) {
			/*
			 * CPU-initiated operation
			 */

			spin_unlock_intrestore(&ops_lock, intmask);

			__cpu_initiated_io(ioop, &irb);
			return;
		}
	}
	spin_unlock_intrestore(&ops_lock, intmask);

	/*
	 * device-initiated operation
	 */
	dev = find_device_by_sch(IO_INT_CODE->ssid);
	BUG_ON(IS_ERR(dev));

	BUG_ON(test_sch(dev->sch, &irb));

	if (dev->dev->interrupt)
		dev->dev->interrupt(dev, &irb);
	else
		__dev_initiated_io(dev, &irb);
}
