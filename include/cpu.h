#ifndef __CPU_H
#define __CPU_H

static inline u64 getcpuid()
{
	u64 cpuid = ~0;

	asm("stidp	%0\n"
	:
	  "=m" (cpuid)
	:
	:
	  "memory"
	);

	return cpuid;
}

#endif
