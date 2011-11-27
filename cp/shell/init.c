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
#include <cpu.h>
#include <vcpu.h>
#include <vdevice.h>
#include <mutex.h>
#include <vsprintf.h>
#include <shell.h>

/* This is used to con_printf to the operator about various async events -
 * e.g., user logon
 */
struct console *oper_con;

static void process_cmd(struct virt_sys *sys)
{
	u8 cmd[128];
	int ret;

	// FIXME: read a line from the
	// ret = con_read(sys->con, cmd, 128);
	ret = -1;

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

int shell_start(void *data)
{
	struct virt_sys *sys = data;
	struct virt_cpu *cpu = sys->task->cpu;
	struct datetime dt;

	/*
	 * load guest's address space into the host's PASCE
	 */
	load_as(&sys->as);

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

