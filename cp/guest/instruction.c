/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <sched.h>
#include <shell.h>

static const intercept_handler_t instruction_funcs[256] = {
	[0xb2] = handle_instruction_priv,	/* assorted priv. insts */
};

int handle_instruction(struct virt_sys *sys)
{
	struct virt_cpu *cpu = sys->cpu;
	intercept_handler_t h;
	int err = -EINVAL;

	con_printf(sys->con, "INTRCPT: INST (%04x %08x)\n",
		   cpu->sie_cb.ipa,
		   cpu->sie_cb.ipb);

	h = instruction_funcs[cpu->sie_cb.ipa >> 8];
	if (h)
		err = h(sys);

	return err;
}
