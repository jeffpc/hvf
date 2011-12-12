/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __CPU_H
#define __CPU_H

static inline u64 getcpuid()
{
	u64 cpuid = ~0;

	asm("stidp	0(%1)\n"
	: /* output */
	  "=m" (cpuid)
	: /* input */
	  "a" (&cpuid)
	);

	return cpuid;
}

static inline u16 getcpuaddr()
{
	u16 cpuaddr = ~0;

	asm("stap	0(%1)\n"
	: /* output */
	  "=m" (cpuaddr)
	: /* input */
	  "a" (&cpuaddr)
	);

	return cpuaddr;
}

#define BIT64(x)	(1u << (63-(x)))
#define BIT32(x)	(1u << (31-(x)))

#define set_cr(cr, val)						\
	do {							\
		u64 lval = (val);				\
		asm volatile(					\
			"lctlg	%1,%1,%0\n"			\
		: /* output */					\
		: /* input */					\
		  "m" (lval),					\
		  "i" (cr)					\
		);						\
	} while(0)

#define get_cr(cr)						\
	({							\
		u64 reg;					\
								\
		asm volatile(					\
			"stctg	%1,%1,%0\n"			\
		: /* output */					\
		  "=m" (reg)					\
		: /* input */					\
		  "i" (cr)					\
		);						\
		reg;						\
	 })

#endif
