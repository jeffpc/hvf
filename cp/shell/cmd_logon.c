/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

/*
 *!!! LOGON
 *!! SYNTAX
 *! \tok{\sc LOGON} <userid>
 *!! XATNYS
 *!! AUTH none
 *!! PURPOSE
 *! Log on to a virtual machine.
 */

static int cmd_logon_fail(struct virt_sys *sys, char *cmd, int len)
{
	con_printf(sys->con, "ALREADY LOGGED ON\n");
	return 0;
}
