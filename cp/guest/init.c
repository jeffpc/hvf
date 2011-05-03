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
#include <shell.h>

/* This is used to con_printf to the operator about various async events -
 * e.g., user logon
 */
struct console *oper_con;

static LOCK_CLASS(online_users_lc);
static LIST_HEAD(online_users);
static UNLOCKED_MUTEX(online_users_lock, &online_users_lc);

static int __alloc_guest_devices(struct virt_sys *sys)
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

	return 0;
}

static int __alloc_guest_storage(struct virt_sys *sys)
{
	u64 pages = sys->directory->storage_size >> PAGE_SHIFT;
	struct page *p;

	INIT_LIST_HEAD(&sys->guest_pages);

	while (pages) {
		p = alloc_pages(0, ZONE_NORMAL);
		if (!p)
			continue; /* FIXME: sleep? */

		list_add(&p->guest, &sys->guest_pages);

		pages--;

		dat_insert_page(&sys->as, (u64) page_to_addr(p),
				pages << PAGE_SHIFT);
	}

	return 0;
}

static void process_logon_cmd(struct console *con)
{
	u8 cmd[128];
	int ret;

	ret = con_read(con, cmd, 128);

	if (ret == -1)
		return; /* no lines to read */

	if (!ret)
		return; /* empty line */

	ebcdic2ascii(cmd, ret);

	/*
	 * we got a command to process!
	 */

	ret = invoke_shell_logon(con, (char*) cmd, ret);
	if (!ret)
		return;

	con_printf(con, "NOT LOGGED ON\n");
}

static void process_cmd(struct virt_sys *sys)
{
	u8 cmd[128];
	int ret;

	ret = con_read(sys->con, cmd, 128);

	if (ret == -1)
		return; /* no lines to read */

	if (!ret) {
		con_printf(sys->con, "CP\n");
		return; /* empty line */
	}

	ebcdic2ascii(cmd, ret);

	/*
	 * we got a command to process!
	 */
	ret = invoke_shell_cmd(sys, (char*) cmd, ret);
	switch (ret) {
		case 0:
			/* all fine */
			break;
		case -ENOENT:
			con_printf(sys->con, "Invalid CP command: %s\n", cmd);
			break;
		case -ESUBENOENT:
			con_printf(sys->con, "Invalid CP sub-command: %s\n", cmd);
			break;
		case -EINVAL:
			con_printf(sys->con, "Operand missing or invalid\n");
			break;
		case -EPERM:
			con_printf(sys->con, "Not authorized\n");
			break;
		default:
			con_printf(sys->con, "RC=%d (%s)\n", ret,
				   errstrings[ret]);
			break;
	}
}

static int shell_init(void *data)
{
	struct virt_sys *sys = data;
	struct virt_cpu *cpu;
	struct datetime dt;
	struct page *page;

	page = alloc_pages(0, ZONE_NORMAL);
	BUG_ON(!page);

	cpu = page_to_addr(page);
	sys->task->cpu = cpu;

	memset(cpu, 0, PAGE_SIZE);
	INIT_LIST_HEAD(&cpu->int_io[0]);
	INIT_LIST_HEAD(&cpu->int_io[1]);
	INIT_LIST_HEAD(&cpu->int_io[2]);
	INIT_LIST_HEAD(&cpu->int_io[3]);
	INIT_LIST_HEAD(&cpu->int_io[4]);
	INIT_LIST_HEAD(&cpu->int_io[5]);
	INIT_LIST_HEAD(&cpu->int_io[6]);
	INIT_LIST_HEAD(&cpu->int_io[7]);

	__alloc_guest_storage(sys);
	__alloc_guest_devices(sys);

	/*
	 * load guest's address space into the host's PASCE
	 */
	load_as(&sys->as);

	cpu->cpuid = getcpuid() | 0xFF00000000000000ULL;

	memset(&cpu->sie_cb, 0, sizeof(struct sie_cb));
	cpu->sie_cb.gmsor = 0;
	cpu->sie_cb.gmslm = sys->directory->storage_size;
	cpu->sie_cb.gbea = 1;
	cpu->sie_cb.ecb  = 2;
	cpu->sie_cb.eca  = 0xC1002001U;
	/*
	 * TODO: What about ->scaoh and ->scaol?
	 */

	guest_power_on_reset(sys);

	get_parsed_tod(&dt);
	con_printf(sys->con, "LOGON FOR %s AT %02d:%02d:%02d UTC %04d-%02d-%02d\n",
		   sys->directory->userid, dt.th, dt.tm, dt.ts, dt.dy, dt.dm, dt.dd);

	for (;;) {
		/*
		 *   - process any console input
		 *   - if the guest is running
		 *     - issue any pending interruptions
		 *     - continue executing it
		 *     - process any intercepts from SIE
		 *   - else, schedule()
		 */

		process_cmd(sys);

		if (cpu->state == GUEST_OPERATING)
			run_guest(sys);
		else
			schedule();
	}

	return 0;
}

static void __con_attn(struct console *con)
{
	if (con->sys) {
		/* There's already a user on this console */

		if (!con->sys->task->cpu ||
		    con->sys->task->cpu->state == GUEST_STOPPED)
			return;

		if (!con_read_pending(con))
			return;

		/*
		 * There's a read pending. Generate an interception.
		 */
		atomic_set_mask(CPUSTAT_STOP_INT, &con->sys->task->cpu->sie_cb.cpuflags);
	} else {
		if (!con_read_pending(con))
			return;

		/*
		 * There's a read pending. MUST be a command
		 */
		process_logon_cmd(con);
	}
}

static int shell_con_attn(void *data)
{
	for(;;) {
		schedule();

		for_each_console(__con_attn);
	}

	return 0;
}

void spawn_oper_shell(struct console *con)
{
	struct virt_sys *sys;
	char tn[32];

	oper_con = con;

	sys = malloc(sizeof(struct virt_sys), ZONE_NORMAL);
	BUG_ON(!sys);

	sys->con = con;
	con->sys = sys;

	sys->directory = find_user_by_id(sysconf.oper_userid);
	BUG_ON(IS_ERR(sys->directory));

	sys->print_ts = 1; /* print timestamps */

	snprintf(tn, 32, "%s-vcpu0", sysconf.oper_userid);
	sys->task = create_task(tn, shell_init, sys);
	BUG_ON(IS_ERR(sys->task));

	BUG_ON(IS_ERR(create_task("console-attn", shell_con_attn, NULL)));

	mutex_lock(&online_users_lock);
	list_add_tail(&sys->online_users, &online_users);
	mutex_unlock(&online_users_lock);
}

void spawn_user_shell(struct console *con, struct user *u)
{
	char tname[TASK_NAME_LEN+1];
	struct virt_sys *sys;
	int already_online = 0;

	mutex_lock(&online_users_lock);
	list_for_each_entry(sys, &online_users, online_users) {
		if (sys->directory == u) {
			already_online = 1;
			break;
		}
	}
	mutex_unlock(&online_users_lock);

	if (already_online) {
		con_printf(con, "ALREADY LOGGED ON\n");
		return;
	}

	sys = malloc(sizeof(struct virt_sys), ZONE_NORMAL);
	if (!sys)
		goto err;

	sys->con = con;
	con->sys = sys;

	sys->directory = u;

	sys->print_ts = 1; /* print timestamps */

	snprintf(tname, TASK_NAME_LEN, "%s-vcpu0", u->userid);
	sys->task = create_task(tname, shell_init, sys);
	if (IS_ERR(sys->task))
		goto err_free;

	mutex_lock(&online_users_lock);
	list_add_tail(&sys->online_users, &online_users);
	mutex_unlock(&online_users_lock);
	return;

err_free:
	free(sys);
	con->sys = NULL;
err:
	con_printf(con, "INTERNAL ERROR DURING LOGON\n");
}

void list_users(struct console *con, void (*f)(struct console *con,
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
