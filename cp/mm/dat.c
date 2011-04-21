/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <page.h>
#include <buddy.h>
#include <dat.h>
#include <mm.h>

static void *alloc_table(int order)
{
	struct page *p;

	p = alloc_pages(order, ZONE_NORMAL);
	BUG_ON(!p);
	memset(page_to_addr(p), 0xff, PAGE_SIZE << order);

	return page_to_addr(p);
}

/**
 * dat_insert_page - insert a virt->phy mapping into an address space
 * @as:		address space to add the mapping to
 * @phy:	physical address to add
 * @virt:	virtual address to add
 */
int dat_insert_page(struct address_space *as, u64 phy, u64 virt)
{
	struct dat_rte *region;
	struct dat_ste *segment;
	struct dat_pte *page;
	void *ptr;

	region = as->region_table;

	if (!region) {
		if (!as->segment_table)
			/*
			 * Need to allocate the segment table
			 *
			 * max of 2048 * 8-byte entries = 16 kbytes
			 */
			as->segment_table = alloc_table(2);

		segment = as->segment_table;

		goto walk_segment;
	}

	BUG_ON(DAT_RX(virt)); // FIXME: we don't support storage >2GB

	if (region->origin == 0xfffffffffffffUL) {
		/*
		 * Need to allocate the segment table
		 *
		 * max of 2048 * 8-byte entries = 16 kbytes
		 */
		ptr = alloc_table(2);

		region->origin = ADDR_TO_RTE_ORIGIN((u64) ptr);

		region->tf = 0; /* FIXME: is this right? */
		region->i = 0;
		region->tt = DAT_RTE_TT_RTT;
		region->tl = 3; /* FIXME: is this right? */
		region->__reserved0 = 0;
		region->__reserved1 = 0;
	}

	segment = RTE_ORIGIN_TO_ADDR(region->origin);

walk_segment:
	segment += DAT_SX(virt);

	if (segment->origin == 0x1fffffffffffffUL) {
		/*
		 * Need to allocate the page table
		 *
		 * max of 256 * 8-byte entries = 2048 bytes
		 */
		ptr = alloc_table(0);

		segment->origin = ADDR_TO_STE_ORIGIN((u64) ptr);
		segment->p = 0;
		segment->i = 0;
		segment->c = 0;
		segment->tt = DAT_STE_TT_ST;
		segment->__reserved0 = 0;
		segment->__reserved1 = 0;
		segment->__reserved2 = 0;
	}

	page = STE_ORIGIN_TO_ADDR(segment->origin);
	page += DAT_PX(virt);

	page->pfra = phy >> PAGE_SHIFT;
	page->i = 0;
	page->p = 0;
	page->__zero0 = 0;
	page->__zero1 = 0;
	page->__reserved = 0;

	return 0;
}

void setup_dat(void)
{
	/* nothing to do! */
}

void load_as(struct address_space *as)
{
	struct dat_td cr1;

	BUG_ON(!as->segment_table);

	/*
	 * Load up the PASCE (cr1)
	 */
	memset(&cr1, 0, sizeof(struct dat_td));
	cr1.origin = ((u64)as->segment_table) >> 12;
	cr1.dt = DAT_TD_DT_ST;
	cr1.tl = 3;

	asm volatile(
		"	lctlg	1,1,%0\n"
	: /* output */
	: /* input */
	  "m" (cr1)
	);
}

/* memcpy that pays attention to page boundary crossing; *len contains the
 * number of bytes to copy on entry, and at return, the number of bytes
 * copied */
int __memcpy_tofrom_guest(u64 guest_addr, void *data, u64 *len, int from)
{
	u8 *_data = data;
	u64 host_addr;
	u64 copy_len;
	u64 copied;
	int ret = 0;

	for(copied = 0; *len; ) {
		/* walk the page tables to find the real page frame */
		ret = virt2phy_current(guest_addr, &host_addr);
		if (ret)
			break;

		/* copy this much this time around... until the end of the page, or
		 * the while thing that's left
		 */
		copy_len = min(PAGE_SIZE - (host_addr & PAGE_MASK), *len);

		/* COPY! */
		if (from)
			memcpy(_data + copied, (void*)host_addr, copy_len);
		else
			memcpy((void*)host_addr, _data + copied, copy_len);

		copied += copy_len;
		*len -= copy_len;
	}

	*len = copied;
	return ret;
}
