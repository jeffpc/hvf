/*
 * Copyright (c) 2007-2012 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <interrupt.h>
#include <symtab.h>
#include <sclp.h>
#include <mm.h>

struct stack_frame {
	u64 back_chain;
	u64 reserved;
	u64 gpr[14];  /* GPR 2..GPR 15 */
	u64 fpr[4];   /* FPR 0, 2, 4, 6 */
};

/*
 * NOTE: Be *very* careful not to deref something we're not supposed to, we
 * don't want to recursively PROG.
 */
static void dump_stack(u64 addr)
{
	char buf[64];
	struct stack_frame *cur;
	u64 start, end;
	u64 ra;

	sclp_msg("Stack trace:\n");

	if (addr > memsize)
		return;

	addr &= ~0x7ull;

	start = addr;
	end = (addr & ~PAGE_MASK) + PAGE_SIZE - sizeof(struct stack_frame);

	cur = (struct stack_frame*) addr;

	while(((u64)cur) >= start && ((u64)cur) < end) {
		ra = cur->gpr[12];

		sclp_msg("   %016llx   %-s\n", ra,
			 symtab_lookup(ra, buf, sizeof(buf)));

		if (!cur->back_chain)
			break;

		cur = (struct stack_frame*) cur->back_chain;
	}
}

/*
 * All is lost! All hands abandon ship!
 *
 * Try to write out a message to a special buffer to help debugging, and
 * then stop the CPU.
 */
static void abend(void)
{
	int i;

	sclp_msg("ABEND\n");
	sclp_msg("PROG %04x, ILC %d\n", *PGM_INT_CODE, (*PGM_INT_ILC) >> 1);
	sclp_msg("\n");
	for(i=0; i<8; i++)
		sclp_msg("R%-2d = %016llx        R%-2d = %016llx\n",
			 i, PSA_INT_GPR[i], i+8, PSA_INT_GPR[i+8]);
	sclp_msg("\n");
	sclp_msg("PSW = %016llx %016llx\n",
		*((u64*) PGM_INT_OLD_PSW), *(((u64*) PGM_INT_OLD_PSW)+1));
	sclp_msg("\n");
	dump_stack(PSA_INT_GPR[15]);

	/*
	 * halt the cpu
	 *
	 * NOTE: we don't care about not clobbering registers as when this
	 * code executes, the CPU will be stopped.
	 */
	asm volatile(
		"SR	%r1, %r1	# not used, but should be zero\n"
		"SR	%r3, %r3 	# CPU Address\n"
		"SIGP	%r1, %r3, 0x05	# Signal, order 0x05\n"
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
