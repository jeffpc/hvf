/*
 * Copyright (c) 2007-2011 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __EBCDIC_H
#define __EBCDIC_H

/* charset conversions */
extern u8 ascii2ebcdic_table[256];
extern u8 ebcdic2ascii_table[256];

/* ASCII transforms */
extern u8 ascii2upper_table[256];

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

#define ascii2upper(buf, len)	\
			__translate((buf), (len), ascii2upper_table)

#endif
