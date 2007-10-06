#ifndef __NUCLEUS_H
#define __NUCLEUS_H

#include <config.h>
#include <compiler.h>
#include <types.h>
#include <errno.h>

/* borrowed from Linux */
#define container_of(ptr, type, member) ({                      \
         const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
         (type *)( (char *)__mptr - offsetof(type,member) );})

/* borrowed from Linux */
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define BUG()		do { } while(0)
#define BUG_ON(cond)	do { \
				if (unlikely(cond)) \
					BUG(); \
			} while(0)

/*
 * string.h equivalents
 */
extern void *memset(void *s, int c, size_t n);
extern void *memcpy(void *dst, void *src, int len);
extern int strnlen(const char *s, size_t maxlen);

/*
 * stdio.h equivalents
 */
extern int vprintf(const char *fmt, va_list args)
        __attribute__ ((format (printf, 1, 0)));
extern int printf(const char *fmt, ...)
        __attribute__ ((format (printf, 1, 2)));
extern int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

/*
 * stdarg.h equivalents
 */
#define va_start(ap, last)	__builtin_va_start(ap, last)
#define va_arg(ap, type) 	__builtin_va_arg(ap, type)
#define va_end(ap) 		__builtin_va_end(ap)

#endif
