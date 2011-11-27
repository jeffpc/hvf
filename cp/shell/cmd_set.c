/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

/*
 *!!! SET NOTS
 *!! SYNTAX
 *! \tok{\sc SET} \tok{\sc NOTS}
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Disable the console timestamp printing
 */
static int cmd_set_nots(struct virt_sys *sys, char *cmd, int len)
{
	SHELL_CMD_AUTH(sys, G);

	sys->print_ts = 0;
	return 0;
}

/*
 *!!! SET TS
 *!! SYNTAX
 *! \tok{\sc SET} \tok{\sc TS}
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Enable the console timestamp printing
 */
static int cmd_set_ts(struct virt_sys *sys, char *cmd, int len)
{
	SHELL_CMD_AUTH(sys, G);

	sys->print_ts = 1;
	return 0;
}

static struct cpcmd cmd_tbl_set[] = {
	{"NOTS",	cmd_set_nots,		NULL},
	{"TS",		cmd_set_ts,		NULL},
	{"",		NULL,			NULL},
};
