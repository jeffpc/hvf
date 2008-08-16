#ifndef __CP_H
#define __CP_H

#include <directory.h>

/*
 * This file should contain only externally (from CP's point of view)
 * visible interfaces.
 */

extern void spawn_oper_cp();
extern int invoke_cp_cmd(struct user *u, char *cmd, int len);

#endif
