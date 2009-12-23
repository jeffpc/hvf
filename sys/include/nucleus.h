#ifndef __NUCLEUS_H
#define __NUCLEUS_H

#include <config.h>
#include <compiler.h>
#include <errno.h>

extern volatile u64 ticks;

extern struct datetime ipltime;

/* The beginning of it all... */
extern void start(u64 __memsize);

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
				asm volatile(".byte 0x00,0x00" : : : "memory"); \
			} while(0)
#define BUG_ON(cond)	do { \
				if (unlikely(cond)) \
					BUG(); \
			} while(0)

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
 * NOTE! This ctype does not handle EOF like the standard C
 * library is required to. (Taken from include/linux/ctype.h)
 */

#define _U      0x01    /* upper */
#define _L      0x02    /* lower */
#define _D      0x04    /* digit */
#define _C      0x08    /* cntrl */
#define _P      0x10    /* punct */
#define _S      0x20    /* white space (space/lf/tab) */
#define _X      0x40    /* hex digit */
#define _SP     0x80    /* hard space (0x20) */

extern unsigned char _ascii_ctype[];

#define __ismask(x) (_ascii_ctype[(int)(unsigned char)(x)])

#define isalnum(c)      ((__ismask(c)&(_U|_L|_D)) != 0)
#define isalpha(c)      ((__ismask(c)&(_U|_L)) != 0)
#define iscntrl(c)      ((__ismask(c)&(_C)) != 0)
#define isdigit(c)      ((__ismask(c)&(_D)) != 0)
#define isgraph(c)      ((__ismask(c)&(_P|_U|_L|_D)) != 0)
#define islower(c)      ((__ismask(c)&(_L)) != 0)
#define isprint(c)      ((__ismask(c)&(_P|_U|_L|_D|_SP)) != 0)
#define ispunct(c)      ((__ismask(c)&(_P)) != 0)
#define isspace(c)      ((__ismask(c)&(_S)) != 0)
#define isupper(c)      ((__ismask(c)&(_U)) != 0)
#define isxdigit(c)     ((__ismask(c)&(_D|_X)) != 0)

/*
 * string.h equivalents
 */
#define memset(s,c,n)	__builtin_memset((s),(c),(n))
#define memcmp(d,s,l)	__builtin_memcmp((d),(s),(l))
#define memcpy(d,s,l)	__builtin_memcpy((d),(s),(l))
extern size_t strnlen(const char *s, size_t count);
extern int strcmp(const char *cs, const char *ct);
extern int strncmp(const char *cs, const char *ct, int len);
extern int strcasecmp(const char *s1, const char *s2);
extern char *strncpy(char *dest, const char *src, size_t count);

static inline unsigned char toupper(unsigned char c)
{
	/*
	 * TODO: This would break if we ever tried to compile within an EBCDIC
	 * environment
	 */
	if ((c >= 'a') && (c <= 'z'))
		c += 'A'-'a';
	return c;
}

static inline unsigned char tolower(unsigned char c)
{
	/*
	 * TODO: This would break if we ever tried to compile within an EBCDIC
	 * environment
	 */
	if ((c >= 'A') && (c <= 'Z'))
		c -= 'A'-'a';
	return c;
}

/*
 * stdio.h equivalents
 */
struct console;

extern int vprintf(struct console *con, const char *fmt, va_list args)
        __attribute__ ((format (printf, 2, 0)));
extern int snprintf(char *buf, int len, const char *fmt, ...)
        __attribute__ ((format (printf, 3, 4)));
extern int con_printf(struct console *con, const char *fmt, ...)
        __attribute__ ((format (printf, 2, 3)));
extern int vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
        __attribute__ ((format (printf, 3, 0)));

/*
 * stdarg.h equivalents
 */
#define va_start(ap, last)	__builtin_va_start(ap, last)
#define va_arg(ap, type)	__builtin_va_arg(ap, type)
#define va_end(ap)		__builtin_va_end(ap)

#endif
