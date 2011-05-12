/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <interrupt.h>
#include <sched.h>

/*
 * This is the tick counter, it starts at 0 when we initialize the nucleus
 */
volatile u64 ticks;

void set_timer(void)
{
	u64 time = 1000 * 1000 * CLK_MICROSEC / HZ;

	asm volatile(
		"spt	%0\n"		/* set timer value */
	: /* output */
	: /* input */
	  "m" (time)
	);
}

void __ext_int_handler(void)
{
	if (*EXT_INT_CODE == 0x1005) {
		/*
		 * This is the timer, let's call the scheduler.
		 */

		ticks++;

		set_timer();

		/*
		 * No need to save the registers, the assembly stub that
		 * called this function already saved them at PSA_INT_GPR
		 */
		if (!(ticks % SCHED_TICKS_PER_SLICE)) {
			__schedule(EXT_INT_OLD_PSW, TASK_SLEEPING);

			/* unreachable */
			BUG();
		}
	}
}

