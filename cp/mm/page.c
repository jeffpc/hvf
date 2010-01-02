/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <list.h>
#include <page.h>
#include <mm.h>

/*
 * Main storage size
 */
u64 memsize;

/*
 * Initialize fields in a struct page
 */
static void __init_page(struct page *page)
{
	INIT_LIST_HEAD(&page->buddy);
}

/*
 * Initialize struct page for each available page
 */
void init_pages(void)
{
	u64 pnum;

	for(pnum=0; pnum < (memsize>>PAGE_SHIFT); pnum++)
		__init_page(page_num_to_ptr(pnum));
}

