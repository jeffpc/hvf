#include <directory.h>
#include <dat.h>
#include <sched.h>
#include <disassm.h>

struct cpcmd {
	const char *name;

	/* handle function pointer */
	int (*fnx)(struct user *u, char *cmd, int len);

	/* sub-command handler table */
	struct cpcmd *sub;
};

/*
 * We use includes here to avoid namespace polution with all the sub-command
 * handler functions
 */
#include "cmd_beginstop.c"
#include "cmd_display.c"
#include "cmd_query.c"

static struct cpcmd commands[] = {
	{"BEGIN", cmd_begin, NULL},
	{"DISPLAY", NULL, cmd_tbl_display},
	{"QUERY", cmd_query, NULL},
	{"STOP", cmd_stop, NULL},
	{NULL, NULL, NULL},
};

static int __invoke_cp_cmd(struct cpcmd *t, struct user *u, char *cmd, int len)
{
	int i, ret;

	for(i=0; t[i].name; i++) {
		const char *inp = cmd, *exp = t[i].name;
		int match_len = 0;

		while(*inp && *exp && (match_len < len) && (*inp == *exp)) {
			match_len++;
			inp++;
			exp++;
		}

		/* doesn't even begin the same */
		if (!match_len)
			continue;

		/*
		 * the next char in the input is...
		 */
		if (cmd[match_len] == ' ')
			/*
			 * command was given arguments - skip over the
			 * delimiting space
			 */
			match_len++;
		else if (cmd[match_len] != '\0')
			/*
			 * command mis-match, try the next one
			 */
			continue;

		if (t[i].sub) {
			ret = __invoke_cp_cmd(t[i].sub, u, cmd + match_len, len - match_len);
			return (ret == -ENOENT) ? -ESUBENOENT : ret;
		}

		return t[i].fnx(u, cmd + match_len, len - match_len);
	}

	return -ENOENT;
}

int invoke_cp_cmd(struct user *u, char *cmd, int len)
{
	return __invoke_cp_cmd(commands, u, cmd, len);
}
