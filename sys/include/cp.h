#ifndef __CP_H
#define __CP_H

#include <directory.h>

#define CP_CMD_MAX_LEN	8

/*
 * This file should contain only externally (from CP's point of view)
 * visible interfaces.
 */
extern struct console *oper_con;

extern void spawn_oper_cp(struct console *con);
extern void spawn_user_cp(struct console *con, struct user *u);
extern int invoke_cp_cmd(struct virt_sys *sys, char *cmd, int len);
extern int invoke_cp_logon(struct console *con, char *cmd, int len);

extern void list_users(struct console *con, void (*f)(struct console *con,
						      struct virt_sys *sys));

/* All the different ways to reset the system */
extern void guest_power_on_reset(struct virt_sys *sys);
extern void guest_system_reset_normal(struct virt_sys *sys);
extern void guest_system_reset_clear(struct virt_sys *sys);
extern void guest_load_normal(struct virt_sys *sys);
extern void guest_load_clear(struct virt_sys *sys);

extern void run_guest(struct virt_sys *sys);
extern void handle_interception(struct virt_sys *sys);

extern int handle_instruction(struct virt_sys *sys);
extern int handle_instruction_priv(struct virt_sys *sys);

typedef int (*intercept_handler_t)(struct virt_sys *sys);

#endif
