/*
 * (C) Copyright 2007-2012  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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
#include <cpu.h>
#include <vcpu.h>
#include <vdevice.h>
#include <mutex.h>
#include <vsprintf.h>
#include <shell.h>
#include <guest.h>

static LOCK_CLASS(online_users_lc);
static LIST_HEAD(online_users);
static UNLOCKED_MUTEX(online_users_lock, &online_users_lc);

static LOCK_CLASS(guest_interrupt_queue_lc);

static LOCK_CLASS(virt_devs_lc);

static void alloc_guest_devices(struct virt_sys *sys)
{
	struct directory_vdev *cur;
	int ret;
	int i;

	mutex_init(&sys->virt_devs_lock, &virt_devs_lc);
	INIT_LIST_HEAD(&sys->virt_devs);

	mutex_lock(&sys->virt_devs_lock);

	i = 0;
	list_for_each_entry(cur, &sys->directory->devices, list) {
		ret = alloc_virt_dev(sys, cur, 0x10000 + i);
		if (ret)
			con_printf(sys->con, "Failed to allocate vdev %04X, "
				   "SCH = %05X (%s)\n", cur->vdev, 0x10000 + i,
				   errstrings[-ret]);
		i++;
	}

	mutex_unlock(&sys->virt_devs_lock);
}

static int alloc_guest_storage(struct virt_sys *sys)
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

static void free_vcpu(struct virt_sys *sys)
{
	struct virt_cpu *cpu = sys->cpu;

	if (sys->task) {
		FIXME("kill the vcpu task");
	}

	assert(list_empty(&cpu->int_io[0]));
	assert(list_empty(&cpu->int_io[1]));
	assert(list_empty(&cpu->int_io[2]));
	assert(list_empty(&cpu->int_io[3]));
	assert(list_empty(&cpu->int_io[4]));
	assert(list_empty(&cpu->int_io[5]));
	assert(list_empty(&cpu->int_io[6]));
	assert(list_empty(&cpu->int_io[7]));

	free_pages(cpu, 0);

	sys->cpu = NULL;
}

static int alloc_vcpu(struct virt_sys *sys)
{
	char tname[TASK_NAME_LEN+1];
	struct virt_cpu *cpu;
	struct page *page;
	int ret;

	page = alloc_pages(0, ZONE_NORMAL);
	if (!page)
		return -ENOMEM;

	cpu = page_to_addr(page);

	memset(cpu, 0, PAGE_SIZE);
	mutex_init(&cpu->int_lock, &guest_interrupt_queue_lc);
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

	sys->cpu = cpu;

	snprintf(tname, 32, "%s-vcpu0", sysconf.oper_userid);
	sys->task = create_task(tname, shell_start, sys);
	if (IS_ERR(sys->task)) {
		ret = PTR_ERR(sys->task);
		sys->task = NULL;
		free_vcpu(sys);
		return ret;
	}

	return 0;
}

static int alloc_console(struct virt_cons *con)
{
	struct page *page;
	int ret;

	con->wlines = alloc_spool();
	if (IS_ERR(con->wlines)) {
		ret = PTR_ERR(con->wlines);
		goto out;
	}

	con->rlines = alloc_spool();
	if (IS_ERR(con->rlines)) {
		ret = PTR_ERR(con->rlines);
		goto out_free;
	}

	page = alloc_pages(0, ZONE_NORMAL);
	if (!page) {
		ret = -ENOMEM;
		goto out_free2;
	}

	con->bigbuf = page_to_addr(page);

	return 0;

out_free2:
	free_spool(con->rlines);
out_free:
	free_spool(con->wlines);
out:
	return ret;
}

static void free_console(struct virt_cons *con)
{
	free_spool(con->rlines);
	free_spool(con->wlines);
	free_pages(con->bigbuf, 0);
}

struct virt_sys *guest_create(char *name, struct device *rcon)
{
	struct virt_sys *sys;
	int already_online = 0;
	int ret;

	/* first, check that we're not already online */
	mutex_lock(&online_users_lock);
	list_for_each_entry(sys, &online_users, online_users) {
		if (!strcmp(sys->directory->userid, name)) {
			already_online = 1;
			break;
		}
	}
	mutex_unlock(&online_users_lock);

	if (already_online)
		return NULL;

	sys = malloc(sizeof(struct virt_sys), ZONE_NORMAL);
	if (!sys)
		return NULL;

	init_circbuf(&sys->crws, struct crw, NUM_CRWS);

	if (alloc_console(&sys->console))
		goto free;

	sys->con = &sys->console;
	sys->con->dev = rcon;

	sys->directory = find_user_by_id(name);
	if (IS_ERR(sys->directory))
		goto free_cons;

	alloc_guest_devices(sys);

	ret = alloc_guest_storage(sys);
	assert(!ret);

	ret = alloc_vcpu(sys);
	assert(!ret);

	sys->print_ts = 1; /* print timestamps */

	mutex_lock(&online_users_lock);
	list_add_tail(&sys->online_users, &online_users);
	mutex_unlock(&online_users_lock);

	return sys;

free_cons:
	free_console(&sys->console);
free:
	free(sys);
	return NULL;
}

void list_users(struct virt_cons *con, void (*f)(struct virt_cons *con,
						 struct virt_sys *sys))
{
	struct virt_sys *sys;

	if (!f)
		return;

	mutex_lock(&online_users_lock);
	list_for_each_entry(sys, &online_users, online_users)
		f(con, sys);
	mutex_unlock(&online_users_lock);
}
