#include <directory.h>
#include <vdevice.h>
#include <dat.h>
#include <mm.h>
#include <sched.h>
#include <disassm.h>
#include <cp.h>
#include <cpu.h>

struct cpcmd {
	const char name[CP_CMD_MAX_LEN];

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
		case 0x3215:	return "CONS";
		case 0x3278:	return "GRAF";
		case 0x3390:	return "DASD";
		case 0x3505:	return "RDR";
		case 0x3525:	return "PUN";

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
#include "cmd_system.c"
#include "cmd_query.c"
#include "cmd_store.c"

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

	{"IPL",		cmd_ipl,		NULL},
	{"IP",		cmd_ipl,		NULL},
	{"I",		cmd_ipl,		NULL},

	{"QUERY",	cmd_query,		NULL},
	{"QUER",	cmd_query,		NULL},
	{"QUE",		cmd_query,		NULL},
	{"QU",		cmd_query,		NULL},
	{"Q",		cmd_query,		NULL},

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

static int __invoke_cp_cmd(struct cpcmd *t, struct virt_sys *sys, char *cmd, int len)
{
	int i, ret;

	for(i=0; t[i].name[0]; i++) {
		const char *inp = cmd, *exp = t[i].name;
		int match_len = 0;

		while(*inp && *exp && (match_len < len) && (toupper(*inp) == *exp)) {
			match_len++;
			inp++;
			exp++;
		}

		/* doesn't match */
		if (match_len != strnlen(t[i].name, CP_CMD_MAX_LEN))
			continue;

		/*
		 * the next char in the input is...
		 */
		while((cmd[match_len] == ' ' || cmd[match_len] == '\t') &&
		      (match_len < len))
			/*
			 * command was given arguments - skip over the
			 * delimiting space
			 */
			match_len++;

		if (t[i].sub) {
			ret = __invoke_cp_cmd(t[i].sub, sys, cmd + match_len, len - match_len);
			return (ret == -ENOENT) ? -ESUBENOENT : ret;
		}

		return t[i].fnx(sys, cmd + match_len, len - match_len);
	}

	return -ENOENT;
}

int invoke_cp_cmd(struct virt_sys *sys, char *cmd, int len)
{
	return __invoke_cp_cmd(commands, sys, cmd, len);
}
