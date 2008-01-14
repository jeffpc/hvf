#include <page.h>
#include <buddy.h>
#include <dat.h>
#include <mm.h>

static struct address_space nucleus_as;

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
	struct page *p;

	region = as->region_table;

	BUG_ON(DAT_RX(virt)); // FIXME: we don't support storage >2GB

	if (region->origin == 0xfffffffffffffUL) {
		p = alloc_pages(0, ZONE_NORMAL);
		BUG_ON(!p);
		memset(page_to_addr(p), 0xff, PAGE_SIZE);
		
		region->origin = ADDR_TO_RTE_ORIGIN((u64) page_to_addr(p));
		region->tf = 0; /* FIXME: is this right? */
		region->i = 0;
		region->tt = DAT_RTE_TT_RTT;
		region->tl = 0; /* FIXME: is this right? */
		region->__reserved0 = 0;
		region->__reserved1 = 0;
	}

	segment = RTE_ORIGIN_TO_ADDR(region->origin);
	segment += DAT_SX(virt);

	if (segment->origin == 0x1fffffffffffffUL) {
		p = alloc_pages(0, ZONE_NORMAL);
		BUG_ON(!p);
		memset(page_to_addr(p), 0xff, PAGE_SIZE);

		segment->origin = ADDR_TO_STE_ORIGIN((u64) page_to_addr(p));
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

/*
 * Note that we are lazy here, and we'll just build the entries for all the
 * storage that's installed.
 */
void setup_dat()
{
	u64 cur_addr;
	struct page *p;

	p = alloc_pages(0, ZONE_NORMAL);
	BUG_ON(!p);

	nucleus_as.region_table = page_to_addr(p);
	memset(nucleus_as.region_table, 0xff, PAGE_SIZE);

	for(cur_addr = 0; cur_addr < memsize; cur_addr += PAGE_SIZE)
		dat_insert_page(&nucleus_as, cur_addr, cur_addr);
}
