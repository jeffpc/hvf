/*
 * Copyright (c) 2007-2010 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#ifndef __CLOCK_H
#define __CLOCK_H

/*
 * The TOD hardware adds 1 to the 51st bit every microsecond
 * The timer hardware subtracts 1 from the 51st bit every microsecond
 */
#define CLK_MICROSEC		0x1000UL
#define CLK_SEC			0xf4240000ULL
#define CLK_MIN			0x3938700000ULL
#define CLK_HOUR		0xd693a400000ULL
#define CLK_DAY			0x141dd76000000ULL
#define CLK_YEAR		0x1cae8c13e000000ULL
#define CLK_FOURYEARS		0x72ce4e26e000000ULL

struct datetime {
	short int dy, dm, dd;
	short int th, tm, ts;
	u32 tmicro;
};

extern struct datetime *parse_tod(struct datetime *dt, u64 tod);

static inline int get_tod(u64 *tod)
{
	int cc;

	asm volatile(
#if 0
		/*
		 * STCKF was added to the z9, and therefore generates a
		 * operation exception on any pre-z9 hardware.
		 *
		 * GNU as doesn't like stckf mnemonic, so let's just invoke
		 * it by hand.
		 */
		".insn	s,0xb27c0000,%0\n"
#else
		"stck	%0\n"
#endif
		"ipm	%1\n"
		"srl	%1,28\n"
	: /* output */
	  "=m" (*tod),
	  "=d" (cc)
	: /* input */
	: /* clobber */
	  "cc"
	);

	return cc;
}

static inline int get_parsed_tod(struct datetime *dt)
{
	u64 tod;
	int ret;

	ret = get_tod(&tod);
	if (!ret)
		parse_tod(dt, tod);

	return ret;
}

static inline int leap_year(int year)
{
	return (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0);
}

#endif
