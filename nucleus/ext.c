#include <interrupt.h>
#include <sched.h>

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

		set_timer();

		/*
		 * No need to save the registers, the assembly stub that
		 * called this function already saved them at PSA_INT_GPR
		 */
		schedule();

		/* unreachable */
 		BUG();
	}
}

