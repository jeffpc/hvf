#ifndef __MM_H
#define __MM_H

extern u64 memsize;

/* Turn Low-address protection on */
static inline void lap_on()
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
