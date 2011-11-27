/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <directory.h>
#include <vcpu.h>
#include <vdevice.h>
#include <dat.h>
#include <mm.h>
#include <sched.h>
#include <disassm.h>
#include <cpu.h>
#include <vsprintf.h>
#include <shell.h>
#include <guest.h>
#include <vcpu.h>

struct cpcmd {
	const char name[SHELL_CMD_MAX_LEN];

	/* handle function pointer */
	int (*fnx)(struct virt_sys *sys, char *cmd, int len);

	/* sub-command handler table */
	struct cpcmd *sub;
};

/*
 * Map a device type to a nice to display name
 */
static char* type2name(u16 type)
{
	switch (type) {
		case 0x1403:	return "PRT";
		case 0x1732:	return "OSA";
		case 0x3088:	return "CTCA";
		case 0x3215:	return "CONS";
		case 0x3278:	return "GRAF";
		case 0x3505:	return "RDR";
		case 0x3525:	return "PUN";

		/* various dasds */
		case 0x3390:
		case 0x9336:	return "DASD";

		/* various tape drives */
		case 0x3480:
		case 0x3490:
		case 0x3590:	return "TAPE";

		default:	return "????";
	}
}

/*
 * We use includes here to avoid namespace polution with all the sub-command
 * handler functions
 */
#include "cmd_helpers.c"

#include "cmd_beginstop.c"
#include "cmd_display.c"
#include "cmd_enable.c"
#include "cmd_logon.c"
#include "cmd_query.c"
#include "cmd_set.c"
#include "cmd_store.c"
#include "cmd_system.c"

static struct cpcmd commands[] = {
	{"BEGIN",	cmd_begin,		NULL},
	{"BEGI",	cmd_begin,		NULL},
	{"BEG",		cmd_begin,		NULL},
	{"BE",		cmd_begin,		NULL},

	{"DISPLAY",	NULL,			cmd_tbl_display},
	{"DISPLA",	NULL,			cmd_tbl_display},
	{"DISPL",	NULL,			cmd_tbl_display},
	{"DISP",	NULL,			cmd_tbl_display},
	{"DIS",		NULL,			cmd_tbl_display},
	{"DI",		NULL,			cmd_tbl_display},
	{"D",		NULL,			cmd_tbl_display},

	{"ENABLE",	cmd_enable,		NULL},
	{"ENABL",	cmd_enable,		NULL},
	{"ENAB",	cmd_enable,		NULL},
	{"ENA",		cmd_enable,		NULL},

	{"IPL",		cmd_ipl,		NULL},
	{"IP",		cmd_ipl,		NULL},
	{"I",		cmd_ipl,		NULL},

	{"LOGIN",	cmd_logon_fail,		NULL},
	{"LOGON",	cmd_logon_fail,		NULL},
	{"LOGO",	cmd_logon_fail,		NULL},
	{"LOGI",	cmd_logon_fail,		NULL},
	{"LOG",		cmd_logon_fail,		NULL},
	{"LO",		cmd_logon_fail,		NULL},
	{"L",		cmd_logon_fail,		NULL},

	{"QUERY",	NULL,			cmd_tbl_query},
	{"QUER",	NULL,			cmd_tbl_query},
	{"QUE",		NULL,			cmd_tbl_query},
	{"QU",		NULL,			cmd_tbl_query},
	{"Q",		NULL,			cmd_tbl_query},

	{"SET",		NULL,			cmd_tbl_set},

	{"STOP",	cmd_stop,		NULL},

	{"STORE",	NULL,			cmd_tbl_store},
	{"STOR",	NULL,			cmd_tbl_store},
	{"STO",		NULL,			cmd_tbl_store},

	{"SYSTEM",	cmd_system,		NULL},
	{"SYSTE",	cmd_system,		NULL},
	{"SYST",	cmd_system,		NULL},
	{"SYS",		cmd_system,		NULL},
	{"",		NULL,			NULL},
};

static int __invoke_shell_cmd(struct cpcmd *t, struct virt_sys *sys, char *cmd, int len)
{
	char *orig = cmd;
	int i, ret;
	char *tok;

	tok = strmsep(&cmd, " ");
	if (!tok)
		return -ENOENT;

	for(i=0; t[i].name[0]; i++) {
		if (strcasecmp(t[i].name, tok))
			continue;

		len -= cmd - orig;

		if (!t[i].sub)
			return t[i].fnx(sys, cmd, len);

		ret = __invoke_shell_cmd(t[i].sub, sys, cmd, len);
		return (ret == -ENOENT) ? -ESUBENOENT : ret;
	}

	return -ENOENT;
}

int invoke_shell_cmd(struct virt_sys *sys, char *cmd, int len)
{
	return __invoke_shell_cmd(commands, sys, cmd, len);
}
