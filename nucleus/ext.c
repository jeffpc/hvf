#include <interrupt.h>
#include <sched.h>

/*
 * This is the tick counter, it starts at 0 when we initialize the nucleus
 */
u64 ticks;

void set_timer()
{
	u64 time = 1000 * 1000 * CLK_MICROSEC / HZ;
	
	asm volatile(
		"spt	%0\n"		/* set timer value */
	: /* output */
	: /* input */
	  "m" (time)
	);
}
 
void __ext_int_handler()
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
		__schedule(EXT_INT_OLD_PSW);

		/* unreachable */
 		BUG();
	}
}

