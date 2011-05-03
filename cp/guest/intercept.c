/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <directory.h>
#include <sched.h>
#include <dat.h>
#include <shell.h>

static int handle_noop(struct virt_sys *sys)
{
	return 0;
}

static int handle_program(struct virt_sys *sys)
{
	con_printf(sys->con, "INTRCPT: PROG\n");
	sys->task->cpu->state = GUEST_STOPPED;
	return 0;
}

static int handle_instruction_and_program(struct virt_sys *sys)
{
	con_printf(sys->con, "INTRCPT: INST+PROG\n");
	sys->task->cpu->state = GUEST_STOPPED;
	return 0;
}

static int handle_ext_req(struct virt_sys *sys)
{
	con_printf(sys->con, "INTRCPT: EXT REQ\n");
	sys->task->cpu->state = GUEST_STOPPED;
	return 0;
}

static int handle_ext_int(struct virt_sys *sys)
{
	con_printf(sys->con, "INTRCPT: EXT INT\n");
	sys->task->cpu->state = GUEST_STOPPED;
	return 0;
}

static int handle_io_req(struct virt_sys *sys)
{
	con_printf(sys->con, "INTRCPT: IO REQ\n");
	sys->task->cpu->state = GUEST_STOPPED;
	return 0;
}

static int handle_wait(struct virt_sys *sys)
{
	con_printf(sys->con, "INTRCPT: WAIT\n");
	return 0;
}

static int handle_validity(struct virt_sys *sys)
{
	con_printf(sys->con, "INTRCPT: VALIDITY\n");
	sys->task->cpu->state = GUEST_STOPPED;
	return 0;
}

static int handle_stop(struct virt_sys *sys)
{
	atomic_clear_mask(CPUSTAT_STOP_INT, &sys->task->cpu->sie_cb.cpuflags);
	return 0;
}

static int handle_oper_except(struct virt_sys *sys)
{
	con_printf(sys->con, "INTRCPT: OPER EXCEPT\n");
	sys->task->cpu->state = GUEST_STOPPED;
	return 0;
}

static int handle_exp_run(struct virt_sys *sys)
{
	con_printf(sys->con, "INTRCPT: EXP RUN\n");
	sys->task->cpu->state = GUEST_STOPPED;
	return 0;
}

static int handle_exp_timer(struct virt_sys *sys)
{
	con_printf(sys->con, "INTRCPT: EXP TIMER\n");
	sys->task->cpu->state = GUEST_STOPPED;
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

void handle_interception(struct virt_sys *sys)
{
	struct virt_cpu *cpu = sys->task->cpu;
	intercept_handler_t h;
	int err = -EINVAL;

	h = intercept_funcs[cpu->sie_cb.icptcode >> 2];
	if (h)
		err = h(sys);

	if (err) {
		cpu->state = GUEST_STOPPED;
		con_printf(sys->con, "Unknown/mis-handled intercept code %02x, err = %d\n",
			   cpu->sie_cb.icptcode, err);
	}
}
