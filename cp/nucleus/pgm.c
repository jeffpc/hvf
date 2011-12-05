/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <interrupt.h>
#include <symtab.h>
#include <sclp.h>
#include <mm.h>

/*
 * NOTE: Be *very* careful not to deref something we're not supposed to, we
 * don't want to recursively PROG.
 */
static void dump_stack(u64 addr)
{
	char buf[64];
	u64 *end = (u64*) ((addr & ~PAGE_MASK) + PAGE_SIZE);
	u64 text_start, text_end;
	u64 *ptr;

	sclp_msg("Stack trace:\n");

	if (addr > memsize)
		return;

	symtab_find_text_range(&text_start, &text_end);

	addr &= ~0x7ull;

	for(ptr=(u64*)addr; ptr<end; ptr++) {
		if ((*ptr < text_start) || (*ptr > text_end))
			continue;

		sclp_msg("   %016llx   %-s\n", *ptr,
			 symtab_lookup(*ptr, buf, sizeof(buf)));
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
