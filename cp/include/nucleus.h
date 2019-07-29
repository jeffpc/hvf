/*
 * (C) Copyright 2007-2019  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __NUCLEUS_H
#define __NUCLEUS_H

#ifndef _ASM

#include <compiler.h>
#include <errno.h>
#include <string.h>
#include <binfmt_elf.h>
#include <sclp.h>
#include <arch.h>

extern volatile u64 ticks;

extern struct datetime ipltime;

/* The beginning of it all... */
extern void start(u64 __memsize, u32 __iplsch, Elf64_Ehdr *__symtab);

/* borrowed from Linux */
#define container_of(ptr, type, member) ({                      \
         const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
         (type *)( (char *)__mptr - offsetof(type,member) );})

/* borrowed from Linux */
#define offsetof(type, member) __builtin_offsetof(type,member)

#define BUG()		do { \
				asm volatile(".byte 0x00,0x00,0x00,0x00" : : : "memory"); \
			} while(0)
#define BUG_ON(cond)	do { \
				if (unlikely(cond)) \
					BUG(); \
			} while(0)
#define assert(cond)	do { \
				if (unlikely(!(cond))) \
					BUG(); \
			} while(0)
#define assert(cond)	do { \
				if (unlikely(!(cond))) \
					BUG(); \
			} while(0)

#define FIXME(fmt, ...)	sclp_msg("FIXME @ " __FILE__ ":%d: " fmt, __LINE__, ##__VA_ARGS__)

#include <config.h>

/*
 * stdio.h equivalents
 */
struct virt_cons;

extern int vprintf(struct virt_cons *con, const char *fmt, va_list args)
        __attribute__ ((format (printf, 2, 0)));
extern int con_printf(struct virt_cons *con, const char *fmt, ...)
        __attribute__ ((format (printf, 2, 3)));

#endif

#endif
