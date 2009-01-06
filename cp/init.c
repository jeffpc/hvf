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

static int __alloc_guest_storage(struct user *user)
{
	u64 pages = user->storage_size >> PAGE_SHIFT;
	struct page *p;

	while (pages) {
		p = alloc_pages(0, ZONE_NORMAL);
		if (!p)
			continue; /* FIXME: sleep? */

		pages--;

		dat_insert_page(&current->guest->as, (u64) page_to_addr(p),
				pages << PAGE_SHIFT);
	}

	return 0;
}

static void process_cmd(struct user *user)
{
	u8 cmd[128];
	int ret;

	ret = con_read(user->con, cmd, 128);

	if (ret == -1)
		return; /* no lines to read */

	if (!ret) {
		con_printf(user->con, "CP\n");
		return; /* empty line */
	}

	ebcdic2ascii(cmd, ret);

	/*
	 * we got a command to process!
	 */
	ret = invoke_cp_cmd(user, (char*) cmd, ret);
	switch (ret) {
		case 0:
			/* all fine */
			break;
		case -ENOENT:
			con_printf(user->con, "Invalid CP command: %s\n", cmd);
			break;
		case -ESUBENOENT:
			con_printf(user->con, "Invalid CP sub-command: %s\n", cmd);
			break;
		case -EINVAL:
			con_printf(user->con, "Operand missing or invalid\n");
			break;
		default:
			con_printf(user->con, "RC=%d\n", ret);
			break;
	}
}

static int cp_init(void *data)
{
	struct user *user = data;
	struct datetime dt;
	struct page *page;

	page = alloc_pages(0, ZONE_NORMAL);
	BUG_ON(!page);

	current->guest = page_to_addr(page);
	memset(current->guest, 0, PAGE_SIZE);

	__alloc_guest_storage(user);

	memset(&current->guest->sie_cb, 0, sizeof(struct sie_cb));
	current->guest->sie_cb.gmsor = 0;
	current->guest->sie_cb.gmslm = user->storage_size;
	current->guest->sie_cb.gbea = 1;
	current->guest->sie_cb.ecb  = 2;
	current->guest->sie_cb.eca  = 0xC1002001U;
	/*
	 * TODO: What about ->scaoh and ->scaol?
	 */

	guest_power_on_reset(user);

	get_parsed_tod(&dt);
	con_printf(user->con, "\nLOGON AT %02d:%02d:%02d UTC %04d-%02d-%02d\n",
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

		process_cmd(user);

		if (current->guest->state == GUEST_OPERATING)
			run_guest(user);
		else
			schedule();
	}
}

/*
 * FIXME: This function should service all the consoles on the system
 */
static int cp_cmd_intercept_gen(void *data)
{
	struct user *user = data;

	for(;;) {
		schedule();

		if (!user->task->guest ||
		    user->task->guest->state == GUEST_STOPPED)
			continue;

		if (!con_read_pending(user->con))
			continue;

		/*
		 * There's a read pending. Generate an interception.
		 */
		atomic_set_mask(CPUSTAT_STOP_INT, &user->task->guest->sie_cb.cpuflags);
	}
}

void spawn_oper_cp()
{
	struct user *u;

	u = find_user_by_id("operator");
	BUG_ON(IS_ERR(u));

	u->task = create_task(cp_init, u);
	BUG_ON(IS_ERR(u->task));

	BUG_ON(IS_ERR(create_task(cp_cmd_intercept_gen, u)));
}
