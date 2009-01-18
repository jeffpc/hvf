#ifndef __LOADER_H
#define __LOADER_H

#define TEMP_BASE	((unsigned char*) 0x400000) /* 4MB */

typedef unsigned long u64;
typedef signed long s64;

typedef unsigned int u32;
typedef signed int s32;

typedef unsigned short u16;
typedef signed short s16;

typedef unsigned char u8;
typedef signed char s8;

extern unsigned char ORB[32];

#define memcpy(d,s,l)	__builtin_memcpy((d), (s), (l))
#define memset(s,c,n)	__builtin_memset((s),(c),(n))

/*
 * It is easier to write this thing in assembly...
 */
extern void __do_io();
extern void PGMHANDLER();

extern void load_nucleus(void);

#endif
