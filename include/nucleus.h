#ifndef __NUCLEUS_H
#define __NUCLEUS_H

#include <config.h>
#include <compiler.h>
#include <types.h>
#include <errno.h>

extern u64 ticks;

/* borrowed from Linux */
#define container_of(ptr, type, member) ({                      \
         const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
         (type *)( (char *)__mptr - offsetof(type,member) );})

/* borrowed from Linux */
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

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

static inline void die(int line)
{
	/* 
	 * halt the cpu
	 * 
	 * NOTE: we don't care about not clobbering registers as when this
	 * code executes, the CPU will be stopped.
	 */
	asm volatile(
		"LR	%%r9, %0		# line number\n"
		"SR	%%r1, %%r1		# not used, but should be zero\n"
		"SR	%%r3, %%r3 		# CPU Address\n"
		"SIGP	%%r1, %%r3, 0x05	# Signal, order 0x05\n"
	: /* out */
	: /* in */
	  "d" (line)
	);

	/*
	 * If SIGP failed, loop forever
	 */
	for(;;);
}

#define BUG()		die(__LINE__) /* FIXME: something should be outputed to console */
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
