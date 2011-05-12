/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __PAGE_H
#define __PAGE_H

#include <list.h>

#define PAGE_SHIFT	12
#define PAGE_SIZE	(1<<PAGE_SHIFT)
#define PAGE_MASK	(PAGE_SIZE-1)

#define PAGE_INFO_BASE	((struct page*) 0x400000)

#define ZONE_NORMAL	0
#define ZONE_LOW	1

#define ZONE_NORMAL_MAX_ORDER	(64-PAGE_SHIFT)
#define ZONE_LOW_MAX_ORDER	(31-PAGE_SHIFT)

/*
 * This structure describes a page of memory
 */
struct page {
	union {
		struct list_head buddy;	/* buddy allocator list */
		struct list_head guest;	/* guest storage list */
	};
};

/*
 * Externs
 */
extern void init_pages(void);

/*
 * Static inlines
 */
static inline struct page *page_num_to_ptr(u64 pnum)
{
	struct page *base = PAGE_INFO_BASE;

	return &base[pnum];
}

static inline void *page_to_addr(struct page *page)
{
	u64 pagenum = (((u64) page) - ((u64) PAGE_INFO_BASE)) / sizeof(struct page);

	return (void*) (pagenum << PAGE_SHIFT);
}

static inline struct page *addr_to_page(void *addr)
{
	struct page *base = PAGE_INFO_BASE;

	return &base[((u64) addr) >> PAGE_SHIFT];
}

static inline int IS_LOW_ZONE(struct page *page)
{
	return ((u64) page_to_addr(page)) < (2UL*1024*1024*1024);
}

static inline int ZONE_TYPE(struct page *page)
{
	return IS_LOW_ZONE(page) ? ZONE_LOW : ZONE_NORMAL;
}

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

static inline u64 ADDR64(void *ptr)
{
	return (u64) ptr;
}

#endif
