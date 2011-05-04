/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <directory.h>
#include <sched.h>
#include <errno.h>
#include <page.h>
#include <buddy.h>
#include <slab.h>
#include <dat.h>
#include <clock.h>
#include <ebcdic.h>
#include <vdevice.h>
#include <cpu.h>
#include <mutex.h>
#include <vsprintf.h>

void alloc_guest_devices(struct virt_sys *sys)
{
	int ret;
	int i;

	INIT_LIST_HEAD(&sys->virt_devs);

	for(i=0; sys->directory->devices[i].type != VDEV_INVAL; i++) {
		ret = alloc_virt_dev(sys, &sys->directory->devices[i],
				     0x10000 + i);
		if (ret)
			con_printf(sys->con, "Failed to allocate vdev %04X, SCH = %05X (%s)\n",
				   sys->directory->devices[i].vdev,
				   0x10000 + i, errstrings[-ret]);
	}
}

int alloc_guest_storage(struct virt_sys *sys)
{
	u64 pages = sys->directory->storage_size >> PAGE_SHIFT;
	struct page *p;

	INIT_LIST_HEAD(&sys->guest_pages);

	while (pages) {
		p = alloc_pages(0, ZONE_NORMAL);
		if (!p)
			return -ENOMEM;

		list_add(&p->guest, &sys->guest_pages);

		pages--;

		dat_insert_page(&sys->as, (u64) page_to_addr(p),
				pages << PAGE_SHIFT);
	}

	return 0;
}

int alloc_vcpu(struct virt_sys *sys)
{
	struct virt_cpu *cpu;
	struct page *page;

	page = alloc_pages(0, ZONE_NORMAL);
	if (!page)
		return -ENOMEM;

	cpu = page_to_addr(page);

	memset(cpu, 0, PAGE_SIZE);
	INIT_LIST_HEAD(&cpu->int_io[0]);
	INIT_LIST_HEAD(&cpu->int_io[1]);
	INIT_LIST_HEAD(&cpu->int_io[2]);
	INIT_LIST_HEAD(&cpu->int_io[3]);
	INIT_LIST_HEAD(&cpu->int_io[4]);
	INIT_LIST_HEAD(&cpu->int_io[5]);
	INIT_LIST_HEAD(&cpu->int_io[6]);
	INIT_LIST_HEAD(&cpu->int_io[7]);

	cpu->cpuid = getcpuid() | 0xFF00000000000000ULL;

	cpu->sie_cb.gmsor = 0;
	cpu->sie_cb.gmslm = sys->directory->storage_size;
	cpu->sie_cb.gbea = 1;
	cpu->sie_cb.ecb  = 2;
	cpu->sie_cb.eca  = 0xC1002001U;
	/*
	 * TODO: What about ->scaoh and ->scaol?
	 */

	sys->task->cpu = cpu;
	return 0;
}

void free_vcpu(struct virt_sys *sys)
{
	struct virt_cpu *cpu = sys->task->cpu;

	BUG_ON(!list_empty(&cpu->int_io[0]));
	BUG_ON(!list_empty(&cpu->int_io[1]));
	BUG_ON(!list_empty(&cpu->int_io[2]));
	BUG_ON(!list_empty(&cpu->int_io[3]));
	BUG_ON(!list_empty(&cpu->int_io[4]));
	BUG_ON(!list_empty(&cpu->int_io[5]));
	BUG_ON(!list_empty(&cpu->int_io[6]));
	BUG_ON(!list_empty(&cpu->int_io[7]));

	free_pages(cpu, 0);

	sys->task->cpu = NULL;
}
