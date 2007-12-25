/*
 * Copyright (c) 2007 Josef 'Jeff' Sipek
 */

#include <mm.h>
#include <slab.h>
#include <page.h>
#include <buddy.h>
#include <io.h>

struct psw {
	u8 bits[8];
	u64 ptr;
};

extern void IO_INT(void);

static struct psw new_io_psw = {
	.bits = { 0x00, 0x04, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00},
	.ptr  = (u64) &IO_INT,
};

/*
 * This is where everything starts
 */
void start()
{
	u64 first_free_page;
	u64 struct_page_bytes;
	struct psw psw;

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
	 * Find & init operator console
	 */
	init_oper_console(OPER_CONSOLE_CCUU);

	/*
	 * Time to enable interrupts => load new psw
	 */
	memcpy((void*) 0x1f0, &new_io_psw, sizeof(struct psw));

	psw.bits[0] = 0x02; // I/O mask
	psw.bits[1] = 0x04; // machine check mask
	psw.bits[2] = 0x00;
	psw.bits[3] = 0x01; // EA
	psw.bits[4] = 0x80; // BA
	psw.bits[5] = 0x00;
	psw.bits[6] = 0x00;
	psw.bits[7] = 0x00;
	asm volatile(
		"	larl	%%r1,L\n"
		"	stg	%%r1,%1\n"
		"	lpswe	%0\n"
		"L:\n"
	: /* output */
	: /* input */
	  "m" (psw),
	  "m" (*(((u8*)&psw) + 8))
	: /* clobbered */
	  "r1"
	);

	printf("HVF version " VERSION);
	printf(" Memory:");
	printf("    %d kB/page", PAGE_SIZE);
	printf("    %llu kB", (unsigned long long) memsize >> 10);
	printf("    %llu pages", (unsigned long long) memsize >> PAGE_SHIFT);
	printf("    PSA for each CPU     0..1024 kB");
	printf("    nucleus              1024..%llu kB",
			(unsigned long long) ((u64)PAGE_INFO_BASE) >> 10);
	printf("    struct page array    %llu..%llu kB",
			(unsigned long long) ((u64)PAGE_INFO_BASE) >> 10,
			(unsigned long long) first_free_page >> 10);
	printf("    generic pages        %llu..%llu kB",
			(unsigned long long) first_free_page >> 10,
			(unsigned long long) memsize >> 10);

	/*
	 * To be or not to be
	 */
	die();
}

