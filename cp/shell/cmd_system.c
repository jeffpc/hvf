/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <ebcdic.h>
#include <binfmt_elf.h>
#include <bcache.h>
#include <edf.h>

/*
 *!!! IPL
 *!! SYNTAX
 *! \tok{\sc IPL} <vdev>
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Perform a ...
 *!! NOTES
 *! \item Not yet implemented.
 *!! SETON
 */
static int cmd_ipl(struct virt_sys *sys, char *cmd, int len)
{
	u64 vdevnum = 0;
	char *c;

	SHELL_CMD_AUTH(sys, G);

	/* get IPL vdev # */
	c = __extract_hex(cmd, &vdevnum);
	if (IS_ERR(c))
		goto nss;

	/* device numbers are 16-bits */
	if (vdevnum & ~0xffff)
		goto nss;

	/* find the virtual device */

#if 0
	for_each_vdev(sys, vdev)
		if (vdev->pmcw.dev_num == (u16) vdevnum) {
			ret = guest_ipl_nss(sys, "ipldev");
			if (ret)
				return ret;

			assert(0);

			return -EINVAL;
		}
#endif

nss:
	/* device not found */
	return guest_ipl_nss(sys, cmd);
}

/*!!! SYSTEM CLEAR
 *!! SYNTAX
 *! \tok{\sc SYStem} \tok{\sc CLEAR}
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Identical to reset-clear button on a real mainframe.
 *
 *!!! SYSTEM RESET
 *!p >>--SYSTEM--RESET-------------------------------------------------------------><
 *!! SYNTAX
 *! \tok{\sc SYStem} \tok{\sc RESET}
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Identical to reset-normal button on a real mainframe.
 *
 *!!! SYSTEM RESTART
 *!! SYNTAX
 *! \tok{\sc SYStem} \tok{\sc RESTART}
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Perform a restart operation.
 *!! NOTES
 *! \item Not yet implemented.
 *!! SETON
 *
 *!!! SYSTEM STORE
 *!! SYNTAX
 *! \tok{\sc SYStem} \tok{\sc STORE}
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Perform a ...
 *!! NOTES
 *! \item Not yet implemented.
 *!! SETON
 */
static int cmd_system(struct virt_sys *sys, char *cmd, int len)
{
	SHELL_CMD_AUTH(sys, G);

	if (!strcasecmp(cmd, "CLEAR")) {
		guest_system_reset_clear(sys);
		con_printf(sys->con, "STORAGE CLEARED - SYSTEM RESET\n");
	} else if (!strcasecmp(cmd, "RESET")) {
		guest_system_reset_normal(sys);
		con_printf(sys->con, "SYSTEM RESET\n");
	} else if (!strcasecmp(cmd, "RESTART")) {
		con_printf(sys->con, "SYSTEM RESTART is not yet supported\n");
	} else if (!strcasecmp(cmd, "STORE")) {
		con_printf(sys->con, "SYSTEM STORE is not yet supported\n");
	} else
		con_printf(sys->con, "SYSTEM: Unknown variable '%s'\n", cmd);

	return 0;
}
