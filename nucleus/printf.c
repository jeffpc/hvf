/*
 * Copyright (c) 2007 Josef 'Jeff' Sipek
 */

#include <channel.h>
#include <io.h>
#include <ebcdic.h>
#include <slab.h>

static u32 oper_console_ssid;

/*
 * Find & initialize the operator console
 *
 * This function figures out the operator console subchannel ID, and then
 * then stores it in `oper_console_ssid' for printf's use in the future.
 */
void init_oper_console(u16 target_ccuu)
{
	struct schib schib;
	u32 sch;
	int cc;

	memset(&schib, 0, sizeof(struct schib));

	/*
	 * For each possible subchannel id...
	 */
	for(sch = 0x10000; sch <= 0x1ffff; sch++) {
		/*
		 * ...call store subchannel, to find out whether or not
		 * there is a device
		 */
		asm volatile(
			"sr	%%r1,%%r1\n"
			"a	%%r1,0(%%r1,%3)\n"
			"stsch	%1\n"
			"ipm	%0\n"
			"srl	%0,28\n"
			: /* output */
			  "=d" (cc),
			  "=m" (schib)
			: /* input */
			  "m" (sch),
			  "a" (&sch)
			: /* clobbered */
			  "cc", "r1"
		);

		/* if not, repeat */
		if (cc)
			continue;

		/* 
		 * Found a device @ sch, check that it is the one we are
		 * looking for
		 */
		if (!schib.path_ctl.v ||
		    schib.path_ctl.dev_num != target_ccuu)
			continue;

		/* the device is the one we are looking for... */

		/* if not already enabled, enable it */
		if (schib.path_ctl.e)
			goto save_sch;

		schib.path_ctl.e = 1;

		/*
		 * Use Modify Subchannel to update the enabled bit
		 */
		asm volatile(
			"sr	%%r1,%%r1\n"
			"a	%%r1,0(%%r1,%3)\n"
			"msch	%1\n"
			"ipm	%0\n"
			"srl	%0,28\n"
			: /* output */
			  "=d" (cc)
			: /* input */
			  "m" (schib),
			  "m" (sch),
			  "a" (&sch)
			: /* clobbered */
			  "cc", "r1"
		);

		/*
		 * if there were no errors, save the subchannel id, and
		 * bail
		 */
		if (!cc)
			goto save_sch;

		/* otherwise, try the next id */
	}

	/*
	 * If we didn't find the device we were looking for, die a horrible
	 * death :)
	 */
	BUG();

save_sch:
	oper_console_ssid = sch;
}

static void __start_console_io(char *buf, int len)
{
	struct io_op ioop;
	struct ccw ccw;
	int ret;

	ioop.ssid = oper_console_ssid;
	ioop.handler = NULL;
	ioop.dtor = NULL;

	memset(&ioop.orb, 0, sizeof(struct orb));
	ioop.orb.lpm = 0xff;
	ioop.orb.addr = (u32) (u64) &ccw;
	ioop.orb.f = 1;

	memset(&ccw, 0, sizeof(struct ccw));
	ccw.cmd = 0x09; // write with auto-carriage return
	ccw.sli = 1;
	ccw.count = len;
	ccw.addr = (u32) (u64) buf;

	ret = submit_io(&ioop, 0);
	BUG_ON(ret);

	/*
	 * FIXME: hack!
	 *
	 * Yes, this is very ugly, but hey, once there is a scheduler, and
	 * we have the luxury of having mutex-type locking primitives, then
	 * we can try to optimize this - something that shouldn't be used
	 * all that much anyway.
	 */
	while(!atomic_read(&ioop.done))
		;
}

int vprintf(const char *fmt, va_list args)
{
	int ret;
	char buf[128];

	ret = vsnprintf(buf, 128, fmt, args);
	if (ret) {
		ascii2ebcdic((u8 *) buf, ret);
		__start_console_io(buf, ret);
	}

	return ret;
}

/*
 * Logic borrowed from Linux's printk
 */
int printf(const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
	r = vprintf(fmt, args);
	va_end(args);

	return r;
}
