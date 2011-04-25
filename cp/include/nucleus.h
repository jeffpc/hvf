/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __NUCLEUS_H
#define __NUCLEUS_H

#include <compiler.h>
#include <errno.h>
#include <string.h>

extern volatile u64 ticks;

extern struct datetime ipltime;

/* The beginning of it all... */
extern void start(u64 __memsize, u32 __iplsch);

/* borrowed from Linux */
#define container_of(ptr, type, member) ({                      \
         const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
         (type *)( (char *)__mptr - offsetof(type,member) );})

/* borrowed from Linux */
#define offsetof(type, member) __builtin_offsetof(type,member)

static inline void lpswe(void *psw)
{
	asm volatile(
		"	lpswe	0(%0)\n"
	: /* output */
	: /* input */
	  "a" (psw)
	: /* clobbered */
	  "cc"
	);
}

#define BUG()		do { \
				asm volatile(".byte 0x00,0x00,0x00,0x00" : : : "memory"); \
			} while(0)
#define BUG_ON(cond)	do { \
				if (unlikely(cond)) \
					BUG(); \
			} while(0)

#include <config.h>

/*
 * This should be as simple as a cast, but unfortunately, the BUG_ON check
 * is there to make sure we never submit a truncated address to the channels
 *
 * In the future, the io code should check if IDA is necessary, and in that
 * case allocate an IDAL & set the IDA ccw flag. Other parts of the system
 * that require 31-bit address should do whatever their equivalent action
 * is.
 */
static inline u32 ADDR31(void *ptr)
{
	u64 ip = (u64) ptr;

	BUG_ON(ip & ~0x7fffffffull);

	return (u32) ip;
}

/*
 * stdio.h equivalents
 */
struct console;

extern int vprintf(struct console *con, const char *fmt, va_list args)
        __attribute__ ((format (printf, 2, 0)));
extern int con_printf(struct console *con, const char *fmt, ...)
        __attribute__ ((format (printf, 2, 3)));

/*
 * stdarg.h equivalents
 */

#endif
