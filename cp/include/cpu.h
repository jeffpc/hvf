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

#endif
