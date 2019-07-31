/*
 * (C) Copyright 2007-2019  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __TYPES_H
#define __TYPES_H

#ifndef _ASM

#define NULL	((void*) 0)

typedef unsigned char		uint8_t;
typedef signed char		int8_t;

typedef unsigned short		uint16_t;
typedef signed short		int16_t;

typedef unsigned int		uint32_t;
typedef signed int		int32_t;

#if defined(_LP64)
typedef unsigned long		uint64_t;
typedef signed long		int64_t;

typedef uint64_t		uintptr_t;
typedef int64_t			intptr_t;
#else
typedef unsigned long long	uint64_t;
typedef signed long long	int64_t;

typedef uint32_t		uintptr_t;
typedef int32_t			intptr_t;
typedef int32_t			ptrdiff_t;
#endif

typedef uintptr_t	size_t;
typedef intptr_t	ssize_t;
typedef intptr_t	ptrdiff_t;

typedef __builtin_va_list va_list;

/*
 * Legacy typedefs
 */
typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;
typedef uint64_t	u64;

typedef int8_t		s8;
typedef int16_t		s16;
typedef int32_t		s32;
typedef int64_t		s64;

/*
 * The MSB is bit 0
 */
#define BIT64(x)	((uint64_t) (1ull << (63-(x))))
#define BIT32(x)	(1u << (31-(x)))
#define BIT16(x)	(1u << (15-(x)))
#define BIT8(x)		(1u << (7-(x)))

/*
 * min/max/clamp/min_t/max_t/clamp_t borrowed from Linux
 */

/*
 * min()/max()/clamp() macros that also do
 * strict type-checking.. See the
 * "unnecessary" pointer comparison.
 */
#define min(x, y) ({				\
	typeof(x) _min1 = (x);			\
	typeof(y) _min2 = (y);			\
	(void) (&_min1 == &_min2);		\
	_min1 < _min2 ? _min1 : _min2; })

#define max(x, y) ({				\
	typeof(x) _max1 = (x);			\
	typeof(y) _max2 = (y);			\
	(void) (&_max1 == &_max2);		\
	_max1 > _max2 ? _max1 : _max2; })

/**
 * clamp - return a value clamped to a given range with strict typechecking
 * @val: current value
 * @min: minimum allowable value
 * @max: maximum allowable value
 *
 * This macro does strict typechecking of min/max to make sure they are of the
 * same type as val.  See the unnecessary pointer comparisons.
 */
#define clamp(val, min, max) ({			\
	typeof(val) __val = (val);		\
	typeof(min) __min = (min);		\
	typeof(max) __max = (max);		\
	(void) (&__val == &__min);		\
	(void) (&__val == &__max);		\
	__val = __val < __min ? __min: __val;	\
	__val > __max ? __max: __val; })

/*
 * ..and if you can't take the strict
 * types, you can specify one yourself.
 *
 * Or not use min/max/clamp at all, of course.
 */
#define min_t(type, x, y) ({			\
	type __min1 = (x);			\
	type __min2 = (y);			\
	__min1 < __min2 ? __min1: __min2; })

#define max_t(type, x, y) ({			\
	type __max1 = (x);			\
	type __max2 = (y);			\
	__max1 > __max2 ? __max1: __max2; })

/**
 * clamp_t - return a value clamped to a given range using a given type
 * @type: the type of variable to use
 * @val: current value
 * @min: minimum allowable value
 * @max: maximum allowable value
 *
 * This macro does no typechecking and uses temporary variables of type
 * 'type' to make all the comparisons.
 */
#define clamp_t(type, val, min, max) ({		\
	type __val = (val);			\
	type __min = (min);			\
	type __max = (max);			\
	__val = __val < __min ? __min: __val;	\
	__val > __max ? __max: __val; })

#endif

#endif
