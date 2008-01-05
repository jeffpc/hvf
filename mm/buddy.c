/*
 * Copyright (c) 2007 Josef 'Jeff' Sipek
 */

#include <list.h>
#include <page.h>
#include <mm.h>
#include <buddy.h>
#include <spinlock.h>

/*
 * Lists of free page ranges
 */
static struct list_head orders[64-PAGE_SHIFT];

static spinlock_t orders_lock = SPIN_LOCK_UNLOCKED;

void init_buddy_alloc(u64 start)
{
	int pages = (memsize - start) >> PAGE_SHIFT;
	int order;
	u64 base = start;
	struct page *p;

	for(order=0; order < (64-PAGE_SHIFT); order++) {
		INIT_LIST_HEAD(&orders[order]);

		if (pages & (1 << order)) {
			p = page_num_to_ptr(base >> PAGE_SHIFT);

			list_add(&p->buddy, &orders[order]);

			base += (PAGE_SIZE << order);
		}
	}
}

static struct page *__alloc_pages(int order)
{
	struct list_head *entry;

	/*
	 * If there is a right-sized series of pages, good!
	 */
	if (!list_empty(&orders[order])) {
		entry = orders[order].next;
		list_del(entry);

		return list_entry(entry, struct page, buddy);
	}

	return NULL;
}

/*
 * Split a given (actual) order allocation into an allocation of wanted
 * order, and return the rest of pages to the unused lists
 *
 * E.g.,
 *
 * pages (base):
 *
 * +---+---+---+---+---+---+---+---+
 * | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
 * +---+---+---+---+---+---+---+---+
 *
 * actual = 3
 * wanted = 1
 *
 * return order 2 range back to free pool:
 *
 *                 +---+---+---+---+
 *                 | 4 | 5 | 6 | 7 |
 *                 +---+---+---+---+
 *
 * return order 1 range back to free pool:
 *
 *         +---+---+
 *         | 2 | 3 |
 *         +---+---+
 *
 * The End. Now, pages 2-7 are back in the free pool, and pages 0&1 are an
 * order 1 allocation.
 *
 */
static void __chip_pages(struct page *base, int actual, int wanted)
{
	struct page *p;

	for(; actual > wanted; actual--) {
		p = &base[1 << (actual-1)];
		list_add(&p->buddy, &orders[actual-1]);
	}
}

struct page *alloc_pages(int order)
{
	struct page *page;
	int gord;
	unsigned long mask;

	spin_lock_intsave(&orders_lock, &mask);

	/* easy way */
	page = __alloc_pages(order);
	if (page)
		goto out;

	/*
	 * There is no page-range of the given order, let's try to find the
	 * smallest one larger and split it
	 */

	for(gord=order+1; gord < (64-PAGE_SHIFT); gord++) {
		page = __alloc_pages(gord);
		if (!page)
			continue;

		__chip_pages(page, gord, order);

		goto out;
	}

	/* alright, totally out of memory */
	page = NULL;

out:
	spin_unlock_intrestore(&orders_lock, mask);

	return page;
}

void free_pages(void *ptr, int order)
{
	struct page *base = addr_to_page(ptr);
	unsigned long mask;

	spin_lock_intsave(&orders_lock, &mask);
	list_add(&base->buddy, &orders[order]);
	spin_unlock_intrestore(&orders_lock, mask);

	/*
	 * TODO: a more complex thing we could try is to coallesce adjecent
	 * page ranges into one higher order one (defrag)
	 */
}
