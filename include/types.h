#ifndef __TYPES_H
#define __TYPES_H

#define NULL	((void*) 0)

typedef unsigned long u64;
typedef signed long s64;

typedef unsigned int u32;
typedef signed int s32;

typedef unsigned short u16;
typedef signed short s16;

typedef unsigned char u8;
typedef signed char s8;

typedef u32 size_t;

typedef __builtin_va_list va_list;

typedef int ptrdiff_t;		/* wha? well, vsnprintf wants it */

#endif
