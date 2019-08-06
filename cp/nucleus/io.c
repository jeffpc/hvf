/*
 * (C) Copyright 2007-2019  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <channel.h>
#include <io.h>
#include <interrupt.h>
#include <device.h>
#include <sched.h>
#include <atomic.h>
#include <spinlock.h>
#include <buddy.h>
#include <sched.h>

/*
 * Helper function to make sure the io_op has everything set right
 */
static int __verify_io_op(struct io_op *ioop)
{
	FIXME("check everything that makes sense to check");
	return 0;
}

static void __reset_reserved_fields(struct io_op *ioop)
{
	ioop->orb.__zero1 = 0;

	ioop->orb.__reserved1 = 0;
	ioop->orb.__reserved2 = 0;
	ioop->orb.__reserved3 = 0;
	ioop->orb.__reserved4 = 0;
	ioop->orb.__reserved5 = 0;
	ioop->orb.__reserved6 = 0;
}

/* NOTE: assumes dev->q_lock is held */
static void __submit_io(struct device *dev)
{
	struct io_op *ioop;
	int err;

	if (dev->q_cur)
		return;

	if (list_empty(&dev->q_out))
		return;

	ioop = list_entry(dev->q_out.next, struct io_op, list);

	err = start_sch(dev->sch, &ioop->orb);
	if (!err) {
		list_del(&ioop->list);
		dev->q_cur = ioop;
	} else
		ioop->err = err;
}

/*
 * Submit an I/O request to a subchannel, and set up everything needed to
 * handle the operation
 */
int submit_io(struct device *dev, struct io_op *ioop, int flags)
{
	static atomic_t op_id_counter;
	unsigned long intmask;
	int err = -EBUSY;

	err = __verify_io_op(ioop);
	if (err)
		return 0;

	/* make sure all reserved fields have the right values */
	__reset_reserved_fields(ioop);

	ioop->err = 0;
	ioop->orb.param = atomic_inc_return(&op_id_counter);
	atomic_set(&ioop->done, 0);

	/* add it to the list of ops */
	spin_lock_intsave(&dev->q_lock, &intmask);
	list_add_tail(&ioop->list, &dev->q_out);

	__submit_io(dev); /* try to submit an IO right now */
	spin_unlock_intrestore(&dev->q_lock, intmask);

	if (flags & CAN_LOOP) {
		while(!atomic_read(&ioop->done))
			;
	} else if (flags & CAN_SLEEP) {
		while(!atomic_read(&ioop->done))
			schedule();
	}

	return 0;
}

/*
 * Initialize the channel I/O subsystem
 */
void init_io(void)
{
	/* enable all I/O interrupt classes */
	enable_io_int_classes(0xff);
}

static int default_io_handler(struct device *dev, struct io_op *ioop, struct irb *irb)
{
	ioop->err = -EAGAIN;

	/* Unit check? */
	if (irb->scsw.dev_status & 0x02) {
		FIXME("we should bail");
		ioop->err = -EUCHECK;
	}

	/* Device End is set, we're done */
	if (irb->scsw.dev_status & 0x04)
		ioop->err = 0;

	return 0;
}

static void __cpu_initiated_io(struct device *dev, struct io_op *ioop, struct irb *irb)
{
	unsigned long intmask;

	ioop->err = test_sch(dev->sch, irb);

	if (!ioop->err && ioop->handler)
		ioop->handler(dev, ioop, irb);
	else if (!ioop->err)
		default_io_handler(dev, ioop, irb);

	/*
	 * We can do this, because the test_sch function sets ->err, and
	 * therefore regardless of ->handler being defined, ->err will have
	 * a reasonable value
	 */
	if (ioop->err == -EAGAIN)
		return; /* leave handler registered */

	/* ...and remove it form the list */
	spin_lock_intsave(&dev->q_lock, &intmask);
	dev->q_cur = NULL;

	__submit_io(dev); /* try to submit another IO */
	spin_unlock_intrestore(&dev->q_lock, intmask);

	/* flag io_op as done... */
	atomic_set(&ioop->done, 1);

	/* call the destructor if there is one */
	if (ioop->dtor)
		ioop->dtor(dev, ioop);
}

static void __dev_initiated_io(struct device *dev, struct irb *irb)
{
}

/*
 * I/O Interrupt handler (C portion)
 */
void __io_int_handler(void)
{
	unsigned long intmask;
	struct io_op *ioop;
	struct device *dev;
	struct irb irb;

	dev = find_device_by_sch(IO_INT_CODE->ssid);
	BUG_ON(IS_ERR(dev));

	spin_lock_intsave(&dev->q_lock, &intmask);
	ioop = dev->q_cur;
	spin_unlock_intrestore(&dev->q_lock, intmask);

	if (ioop && ioop->orb.param == IO_INT_CODE->param &&
	    dev->sch == IO_INT_CODE->ssid) {
		/*
		 * CPU-initiated operation
		 */

		__cpu_initiated_io(dev, ioop, &irb);
		dev_put(dev);
		return;
	}

	/*
	 * device-initiated operation
	 */
	BUG_ON(test_sch(dev->sch, &irb));

	atomic_inc(&dev->attention);

	if (dev->dev->interrupt)
		dev->dev->interrupt(dev, &irb);
	else
		__dev_initiated_io(dev, &irb);
	dev_put(dev);
}
