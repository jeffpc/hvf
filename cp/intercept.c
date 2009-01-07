#include <directory.h>
#include <sched.h>
#include <dat.h>
#include <cp.h>

static int handle_noop(struct user *user)
{
	return 0;
}

static int handle_program(struct user *user)
{
	con_printf(user->con, "INTRCPT: PROG\n");
	current->guest->state = GUEST_STOPPED;
	return 0;
}

static int handle_instruction_and_program(struct user *user)
{
	con_printf(user->con, "INTRCPT: INST+PROG\n");
	current->guest->state = GUEST_STOPPED;
	return 0;
}

static int handle_ext_req(struct user *user)
{
	con_printf(user->con, "INTRCPT: EXT REQ\n");
	current->guest->state = GUEST_STOPPED;
	return 0;
}

static int handle_ext_int(struct user *user)
{
	con_printf(user->con, "INTRCPT: EXT INT\n");
	current->guest->state = GUEST_STOPPED;
	return 0;
}

static int handle_io_req(struct user *user)
{
	con_printf(user->con, "INTRCPT: IO REQ\n");
	current->guest->state = GUEST_STOPPED;
	return 0;
}

static int handle_wait(struct user *user)
{
	con_printf(user->con, "INTRCPT: WAIT\n");
	current->guest->state = GUEST_STOPPED;
	return 0;
}

static int handle_validity(struct user *user)
{
	con_printf(user->con, "INTRCPT: VALIDITY\n");
	current->guest->state = GUEST_STOPPED;
	return 0;
}

static int handle_stop(struct user *user)
{
	current->guest->state = GUEST_STOPPED;
	atomic_clear_mask(CPUSTAT_STOP_INT, &current->guest->sie_cb.cpuflags);
	return 0;
}

static int handle_oper_except(struct user *user)
{
	con_printf(user->con, "INTRCPT: OPER EXCEPT\n");
	current->guest->state = GUEST_STOPPED;
	return 0;
}

static int handle_exp_run(struct user *user)
{
	con_printf(user->con, "INTRCPT: EXP RUN\n");
	current->guest->state = GUEST_STOPPED;
	return 0;
}

static int handle_exp_timer(struct user *user)
{
	con_printf(user->con, "INTRCPT: EXP TIMER\n");
	current->guest->state = GUEST_STOPPED;
	return 0;
}

static const intercept_handler_t intercept_funcs[0x4c >> 2] = {
	[0x00 >> 2] = handle_noop,
	[0x04 >> 2] = handle_instruction,
	[0x08 >> 2] = handle_program,
	[0x0c >> 2] = handle_instruction_and_program,
	[0x10 >> 2] = handle_ext_req,
	[0x14 >> 2] = handle_ext_int,
	[0x18 >> 2] = handle_io_req,
	[0x1c >> 2] = handle_wait,
	[0x20 >> 2] = handle_validity,
	[0x28 >> 2] = handle_stop,
	[0x2c >> 2] = handle_oper_except,
	[0x44 >> 2] = handle_exp_run,
	[0x48 >> 2] = handle_exp_timer,
};

void handle_interception(struct user *user)
{
	intercept_handler_t h;
	int err = -EINVAL;

	h = intercept_funcs[current->guest->sie_cb.icptcode >> 2];
	if (h)
		err = h(user);

	if (err) {
		current->guest->state = GUEST_STOPPED;
		con_printf(user->con, "Unknown/mis-handled intercept code %02x, err = %d\n",
			   current->guest->sie_cb.icptcode, err);
	}
}
