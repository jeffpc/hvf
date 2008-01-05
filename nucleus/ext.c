#include <interrupt.h>

void __ext_int_handler()
{
	if (*EXT_INT_CODE == 0x1005) {
		/*
		 * This is the timer, let's figure out if we should call the
		 * scheduler or not.
		 */
	}
}

