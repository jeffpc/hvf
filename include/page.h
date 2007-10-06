#ifndef __PAGE_H
#define __PAGE_H

#include <list.h>

#define PAGE_SHIFT	12
#define PAGE_SIZE	(1<<PAGE_SHIFT)

#define PAGE_INFO_BASE	((struct page*) 0x400000)

/*
 * This structure describes a page of memory
 */
struct page {
	struct list_head buddy;		/* buddy allocator list */
};

/*
 * Externs
 */
extern void init_pages();

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

#endif
