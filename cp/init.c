#include <directory.h>
#include <sched.h>
#include <errno.h>

static int cp_init(void *data)
{
	struct user *user = data;

	for (;;) {
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
