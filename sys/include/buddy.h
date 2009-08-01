#ifndef __BUDDY_H
#define __BUDDY_H

#include <page.h>

extern void init_buddy_alloc(u64 start);
extern struct page *alloc_pages(int order, int type);
extern void free_pages(void *ptr, int order);

#endif
