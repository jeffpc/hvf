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

extern void spawn_oper_shell(struct console *con);
extern void spawn_user_shell(struct console *con, struct user *u);
extern int invoke_shell_cmd(struct virt_sys *sys, char *cmd, int len);
extern int invoke_shell_logon(struct console *con, char *cmd, int len);

extern void list_users(struct console *con, void (*f)(struct console *con,
						      struct virt_sys *sys));

extern int spool_exec(struct virt_sys *sys, struct virt_device *vdev);

#define SHELL_CMD_AUTH(s,a)	do { \
					if (!((s)->directory->auth & AUTH_##a)) \
						return -EPERM; \
				} while(0)

#endif
