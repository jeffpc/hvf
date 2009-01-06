#ifndef __CP_H
#define __CP_H

#include <directory.h>

/*
 * This file should contain only externally (from CP's point of view)
 * visible interfaces.
 */

extern void spawn_oper_cp();
extern int invoke_cp_cmd(struct user *u, char *cmd, int len);

/* All the different ways to reset the system */
extern void guest_power_on_reset(struct user *user);
extern void guest_system_reset_normal(struct user *user);
extern void guest_system_reset_clear(struct user *user);
extern void guest_load_normal(struct user *user);
extern void guest_load_clear(struct user *user);

extern void run_guest(struct user *user);

#endif
