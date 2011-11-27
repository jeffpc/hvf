/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __SHELL_H
#define __SHELL_H

#include <directory.h>
#include <vcpu.h>
#include <vdevice.h>

#define SHELL_CMD_MAX_LEN	8

/*
 * This file should contain only externally (from CP's point of view)
 * visible interfaces.
 */
extern struct console *oper_con;

extern int shell_start(void *data);

extern int invoke_shell_cmd(struct virt_sys *sys, char *cmd, int len);
extern int invoke_shell_logon(struct console *con, char *cmd, int len);

#define SHELL_CMD_AUTH(s,a)	do { \
					if (!((s)->directory->auth & AUTH_##a)) \
						return -EPERM; \
				} while(0)

#endif
