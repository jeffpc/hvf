#ifndef __EBCDIC_H
#define __EBCDIC_H

extern u8 ascii2ebcdic_table[256];
extern u8 ebcdic2ascii_table[256];

/*
 * Generic translate buffer function.
 */
static inline void __translate(u8 *buf, int len, const u8 *table)
{
	asm volatile(
		"	sr	%%r0,%%r0\n"		/* test byte = 0 */
		"	la	%%r2,0(%0)\n"		/* buffer */
		"	sr	%%r3,%%r3\n"
		"	ar	%%r3,%2\n"		/* length */
		"	la	%%r4,0(%1)\n"		/* table */
		"	tre	%%r2,%%r4\n"
		: /* output */
		: /* input */
		  "a" (buf),
		  "a" (table),
		  "d" (len)
		: /* clobbered */
		  "cc", "r0", "r2", "r3", "r4"
	);
}

#define ascii2ebcdic(buf, len)	\
			__translate((buf), (len), ascii2ebcdic_table)

#endif
