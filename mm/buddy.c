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
static struct list_head orders_normal[64-PAGE_SHIFT];
static struct list_head orders_low[31-PAGE_SHIFT];

static spinlock_t orders_lock = SPIN_LOCK_UNLOCKED;

static void __init_buddy_alloc(u64 base, int pages, int max_order,
			       struct list_head *orders)
{
	int order;
	struct page *p;

	for(order=0; order < max_order; order++) {
		INIT_LIST_HEAD(&orders[order]);

		if (pages & (1 << order)) {
			p = page_num_to_ptr(base >> PAGE_SHIFT);

			list_add(&p->buddy, &orders[order]);

			base += (PAGE_SIZE << order);
		}
	}
}

/*
 * Initialize the buddy allocator. This is rather simple, and the only
 * complication is that we must keep track of storage below 2GB separately
 * since some IO related structures can address only the first 2GB. *sigh*
 */
void init_buddy_alloc(u64 start)
{
	u64 normal_pages;
	u64 low_pages;

	if (memsize > 2UL*1024*1024*1024) {
		/* There are more than 2GB of storage */
		low_pages = (2UL*1024*1024*1024 - start) >> PAGE_SHIFT;
		normal_pages = (memsize - start) >> PAGE_SHIFT;
		normal_pages -= low_pages;
	} else {
		/* All the storage is under the 2GB mark */
		low_pages = (memsize - start) >> PAGE_SHIFT;
		normal_pages = 0;
	}

	/* let's add all the free low storage to the lists */
	__init_buddy_alloc(start, low_pages, ZONE_LOW_MAX_ORDER, orders_low);

	/* now, let's add all the free storage above 2GB */
	__init_buddy_alloc(2UL*1024*1024*1024, normal_pages, ZONE_NORMAL_MAX_ORDER,
			   orders_normal);
}

static struct page *__do_alloc_pages(int order, struct list_head *orders)
{
	struct page *page;

	if (!list_empty(&orders[order])) {
		page = list_first_entry(&orders[order], struct page, buddy);

		list_del(&page->buddy);

		return page;
	}

	return NULL;
}

static struct page *__alloc_pages(int order, int type)
{
	struct page *p;

	/* Do we have to use ZONE_LOW? */
	if (type == ZONE_LOW)
		goto zone_low;

	p = __do_alloc_pages(order, orders_normal);
	if (p)
		return p;

	/* If there was no ZONE_NORMAL page, let's try ZONE_LOW */

zone_low:
	return __do_alloc_pages(order, orders_low);
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
	struct list_head *orders;

	orders = (IS_LOW_ZONE(base) ? orders_low : orders_normal);

	for(; actual > wanted; actual--) {
		p = &base[1 << (actual-1)];
		list_add(&p->buddy, &orders[actual-1]);
	}
}

/**
 * alloc_pages - allocate a series of consecutive pages
 * @order:	allocate 2^order consecutive pages
 * @type:	type of pages (<2GB, or anywhere)
 */
struct page *alloc_pages(int order, int type)
{
	struct page *page;
	int gord;
	unsigned long mask;
	int max_order;

	spin_lock_intsave(&orders_lock, &mask);

	/* easy way */
	page = __alloc_pages(order, type);
	if (page)
		goto out;

	/*
	 * There is no page-range of the given order, let's try to find the
	 * smallest one larger and split it
	 */

	max_order = (type == ZONE_LOW ?
			ZONE_LOW_MAX_ORDER : ZONE_NORMAL_MAX_ORDER);

	for(gord=order+1; gord < max_order; gord++) {
		if (gord >= max_order)
			break;

		page = __alloc_pages(gord, type);
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
	struct list_head *orders;

	orders = IS_LOW_ZONE(base) ? orders_low : orders_normal;

	spin_lock_intsave(&orders_lock, &mask);
	list_add(&base->buddy, &orders[order]);
	spin_unlock_intrestore(&orders_lock, mask);

	/*
	 * TODO: a more complex thing we could try is to coallesce adjecent
	 * page ranges into one higher order one (defrag)
	 */
}
