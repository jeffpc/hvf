/*
 * Copyright (c) 2007 Josef 'Jeff' Sipek
 */

#include <mm.h>
#include <slab.h>
#include <page.h>
#include <buddy.h>
#include <io.h>
#include <sched.h>
#include <device.h>
#include <interrupt.h>

static struct psw new_io_psw = {
	.ea	= 1,
	.ba	= 1,

	.ptr  = (u64) &IO_INT,
};

static struct psw new_ext_psw = {
	.ea	= 1,
	.ba	= 1,

	.ptr	= (u64) &EXT_INT,
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
	 * Register all the device drivers
	 */
	register_drivers();

	/*
	 * Find & init operator console
	 */
	init_oper_console(OPER_CONSOLE_CCUU);

	/*
	 * Time to enable interrupts => load new psw
	 */
	memcpy(IO_INT_NEW_PSW, &new_io_psw, sizeof(struct psw));

	memset(&psw, 0, sizeof(struct psw));
	psw.io	= 1;
	psw.ea	= 1;
	psw.ba	= 1;

	asm volatile(
		"	larl	%%r1,0f\n"
		"	stg	%%r1,%0\n"
		"	lpswe	%1\n"
		"0:\n"
	: /* output */
	  "=m" (psw.ptr)
	: /* input */
	  "m" (psw)
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

	printf(" Devices:");

	/*
	 * Let's discover all the devices attached
	 */
	scan_devices();

	/* Initialize the process scheduler */
	init_sched();

	printf(" Scheduler:");
	printf("    no task max");

	/*
	 * Time to enable more interrupts => load new psw
	 */
	memcpy(EXT_INT_NEW_PSW, &new_ext_psw, sizeof(struct psw));

	memset(&psw, 0, sizeof(struct psw));
	psw.io	= 1;
	psw.ex	= 1;
	psw.ea	= 1;
	psw.ba	= 1;

	asm volatile(
		"	larl	%%r1,0f\n"
		"	stg	%%r1,%0\n"
		"	lpswe	%1\n"
		"0:\n"
	: /* output */
	  "=m" (psw.ptr)
	: /* input */
	  "m" (psw)
	: /* clobbered */
	  "r1"
	);

	/*
	 * Loop until an interrupt takes over
	 */
	for(;;);
}

