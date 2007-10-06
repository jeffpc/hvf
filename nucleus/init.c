/*
 * Copyright (c) 2007 Josef 'Jeff' Sipek
 */

#include <mm.h>
#include <slab.h>
#include <page.h>
#include <buddy.h>
#include <io.h>

/*
 * This is where everything starts
 */
void start()
{
	u64 first_free_page;
	u64 struct_page_bytes;

	/*
	 * We should determine this dynamically
	 */
	memsize = MEMSIZE;

	/*
	 * Initialize struct page entries
	 */
	init_pages();

	/*
	 * Calculate address of the first free page (we may have to round
	 * up)
	 */
	struct_page_bytes = (memsize >> PAGE_SHIFT) * sizeof(struct page);

	first_free_page = (u64) PAGE_INFO_BASE + struct_page_bytes;
	if (struct_page_bytes & (PAGE_SIZE-1))
		first_free_page += PAGE_SIZE - (struct_page_bytes & (PAGE_SIZE-1));

	/*
	 * Initialize the buddy allocator
	 */
	init_buddy_alloc(first_free_page);

	/*
	 * Initialize slab allocator default caches
	 */
	init_slab();

	/*
	 * Initialize the io subsystem
	 */
	init_io();

	/*
	 * To be or not to be
	 */
	die();
}

