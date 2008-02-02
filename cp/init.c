#include <directory.h>
#include <sched.h>
#include <errno.h>
#include <page.h>
#include <buddy.h>
#include <slab.h>
#include <dat.h>
#include <cp.h>
#include <clock.h>

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

static int cp_init(void *data)
{
	struct user *user = data;
	struct datetime dt;

	current->guest = malloc(sizeof(struct guest_state), ZONE_NORMAL);
	BUG_ON(!current->guest);
	memset(current->guest, 0, sizeof(struct guest_state));

	__alloc_guest_storage(user);

	get_parsed_tod(&dt);
	con_printf(user->con, "\nLOGON AT %02d:%02d:%02d UTC %04d-%02d-%02d\n", 
			dt.th, dt.tm, dt.ts, dt.dy, dt.dm, dt.dd);

	for (;;) {
		/*
		 * FIXME:
		 *   - process any console input
		 *   - process any intercepts from SIE
		 *   - issue any pending interruptions
		 *   - check if the VM is in a running state, and if it is,
		 *     continue executing it
		 *   - if there's really nothing to do, schedule()
		 */

		schedule();
	}
}

void spawn_oper_cp()
{
	struct user *u;
	int err;

	u = find_user_by_id("operator");
	BUG_ON(IS_ERR(u));

	err = create_task(cp_init, u);

	BUG_ON(err);
}
