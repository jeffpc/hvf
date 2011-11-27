/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

/*
 *!!! BEGIN
 *!! SYNTAX
 *! \tok{\sc BEgin}
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Starts/resumes virtual machine execution.
 */
static int cmd_begin(struct virt_sys *sys, char *cmd, int len)
{
	SHELL_CMD_AUTH(sys, G);

	sys->task->cpu->state = GUEST_OPERATING;
	return 0;
}

/*
 *!!! STOP
 *!! SYNTAX
 *! \tok{\sc STOP}
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Stops virtual machine execution.
 */
static int cmd_stop(struct virt_sys *sys, char *cmd, int len)
{
	SHELL_CMD_AUTH(sys, G);

	sys->task->cpu->state = GUEST_STOPPED;
	return 0;
}
