#include <sched.h>
#include <cp.h>

static int handle_msch(struct virt_sys *sys)
{
	con_printf(sys->con, "MSCH handler\n");
	return -ENOMEM;
}

static int handle_ssch(struct virt_sys *sys)
{
	con_printf(sys->con, "SSCH handler\n");
	return -ENOMEM;
}

static int handle_stsch(struct virt_sys *sys)
{
	con_printf(sys->con, "STSCH handler\n");
	return -ENOMEM;
}

static const intercept_handler_t instruction_priv_funcs[256] = {
	[0x32] = handle_msch,
	[0x33] = handle_ssch,
	[0x34] = handle_stsch,
};

int handle_instruction_priv(struct virt_sys *sys)
{
	struct virt_cpu *cpu = sys->task->cpu;
	intercept_handler_t h;
	int err = -EINVAL;

	h = instruction_priv_funcs[cpu->sie_cb.ipa & 0xff];
	if (h)
		err = h(sys);

	return err;
}

