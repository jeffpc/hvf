#include <sched.h>
#include <cp.h>

static int handle_msch(struct user *user)
{
	con_printf(user->con, "MSCH handler\n");
	return -ENOMEM;
}

static int handle_ssch(struct user *user)
{
	con_printf(user->con, "SSCH handler\n");
	return -ENOMEM;
}

static int handle_stsch(struct user *user)
{
	con_printf(user->con, "STSCH handler\n");
	return -ENOMEM;
}

static const intercept_handler_t instruction_priv_funcs[256] = {
	[0x32] = handle_msch,
	[0x33] = handle_ssch,
	[0x34] = handle_stsch,
};

int handle_instruction_priv(struct user *user)
{
	intercept_handler_t h;
	int err = -EINVAL;

	h = instruction_priv_funcs[current->guest->sie_cb.ipa & 0xff];
	if (h)
		err = h(user);

	return err;
}

