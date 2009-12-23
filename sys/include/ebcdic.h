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
		"	sgr	%%r0,%%r0\n"		/* test byte = 0 */
		"	la	%%r2,0(%0)\n"		/* buffer */
		"	lgr	%%r3,%2\n"		/* length */
		"	la	%%r4,0(%1)\n"		/* table */
		"0:	tre	%%r2,%%r4\n"
		"	brc	1,0b\n"
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
#define ebcdic2ascii(buf, len)  \
			__translate((buf), (len), ebcdic2ascii_table)

#endif
