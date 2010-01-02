/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <interrupt.h>
#include <ebcdic.h>
#include <vsprintf.h>

static char abend_msg_buf[1024];

/*
 * All is lost! All hands abandon ship!
 *
 * Try to write out a message to a special buffer to help debugging, and
 * then stop the CPU.
 */
static void abend(void)
{
	int ret;

	ret = snprintf(abend_msg_buf, 1024,
		"ABEND       %04x"
		"PSW        ILC %d"
		"%016llx%016llx"
		"GPR             "
		"%016llx%016llx%016llx%016llx"
		"%016llx%016llx%016llx%016llx"
		"%016llx%016llx%016llx%016llx"
		"%016llx%016llx%016llx%016llx",
		*PGM_INT_CODE, (*PGM_INT_ILC) >> 1,
		*((u64*) PGM_INT_OLD_PSW), *(((u64*) PGM_INT_OLD_PSW)+1),
		PSA_INT_GPR[0],  PSA_INT_GPR[1],  PSA_INT_GPR[2],  PSA_INT_GPR[3],
		PSA_INT_GPR[4],  PSA_INT_GPR[5],  PSA_INT_GPR[6],  PSA_INT_GPR[7],
		PSA_INT_GPR[8],  PSA_INT_GPR[9],  PSA_INT_GPR[10], PSA_INT_GPR[11],
		PSA_INT_GPR[12], PSA_INT_GPR[13], PSA_INT_GPR[14], PSA_INT_GPR[15]
	);

	if (ret)
		ascii2ebcdic((u8 *) abend_msg_buf, ret);

	/*
	 * halt the cpu
	 *
	 * NOTE: we don't care about not clobbering registers as when this
	 * code executes, the CPU will be stopped.
	 */
	asm volatile(
		"LR	%%r15, %0		# buffer pointer\n"
		"SR	%%r1, %%r1		# not used, but should be zero\n"
		"SR	%%r3, %%r3 		# CPU Address\n"
		"SIGP	%%r1, %%r3, 0x05	# Signal, order 0x05\n"
	: /* out */
	: /* in */
	  "a" (abend_msg_buf)
	);

	/*
	 * If SIGP failed, loop forever
	 */
	for(;;);
}

void __pgm_int_handler(void)
{
	abend();
}
