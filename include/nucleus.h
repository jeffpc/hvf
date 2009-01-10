#ifndef __NUCLEUS_H
#define __NUCLEUS_H

#include <config.h>
#include <compiler.h>
#include <types.h>
#include <errno.h>

extern volatile u64 ticks;

extern struct datetime ipltime;

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
 * string.h equivalents
 */
#define memset(s,c,n)	__builtin_memset((s),(c),(n))
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
