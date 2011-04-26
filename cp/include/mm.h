/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __MM_H
#define __MM_H

extern u64 memsize;

extern const u8 zeropage[PAGE_SIZE];

/* Turn Low-address protection on */
static inline void lap_on(void)
{
	u64 cr0;

	asm volatile(
		"stctg	0,0,%0\n"	/* get cr0 */
		"oi	%1,0x10\n"	/* enable */
		"lctlg	0,0,%0\n"	/* reload cr0 */
	: /* output */
	: /* input */
	  "m" (cr0),
	  "m" (*(u64*) (((u8*)&cr0) + 4))
	);
}

#endif
