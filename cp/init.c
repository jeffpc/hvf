#include <directory.h>
#include <sched.h>
#include <errno.h>
#include <page.h>
#include <buddy.h>
#include <slab.h>
#include <dat.h>
#include <cp.h>
#include <clock.h>
#include <ebcdic.h>

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
	ret = invoke_cp_cmd(sys, (char*) cmd, ret);
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
		default:
			con_printf(sys->con, "RC=%d\n", ret);
			break;
	}
}

static int cp_init(void *data)
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

	__alloc_guest_storage(sys);

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
	con_printf(sys->con, "LOGON AT %02d:%02d:%02d UTC %04d-%02d-%02d\n",
		   dt.th, dt.tm, dt.ts, dt.dy, dt.dm, dt.dd);

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
}

/*
 * FIXME: This function should service all the consoles on the system
 */
static int cp_cmd_intercept_gen(void *data)
{
	struct virt_sys *sys = data;

	for(;;) {
		schedule();

		if (!sys->task->cpu ||
		    sys->task->cpu->state == GUEST_STOPPED)
			continue;

		if (!con_read_pending(sys->con))
			continue;

		/*
		 * There's a read pending. Generate an interception.
		 */
		atomic_set_mask(CPUSTAT_STOP_INT, &sys->task->cpu->sie_cb.cpuflags);
	}
}

void spawn_oper_cp(struct console *con)
{
	struct virt_sys *sys;

	sys = malloc(sizeof(struct virt_sys), ZONE_NORMAL);
	BUG_ON(!sys);

	sys->con = con;

	sys->directory = find_user_by_id("operator");
	BUG_ON(IS_ERR(sys->directory));

	sys->task = create_task(cp_init, sys);
	BUG_ON(IS_ERR(sys->task));

	BUG_ON(IS_ERR(create_task(cp_cmd_intercept_gen, sys)));
}
