#include <directory.h>

struct cpcmd {
	const char *name;
	int (*f)(struct user *u, char *cmd, int len);
};

static int cmd_query(struct user *u, char *cmd, int len)
{
	con_printf(u->con, "got a query! '%s'\n", cmd);
	return 0;
}

static struct cpcmd commands[] = {
	{"QUERY", cmd_query},
	{NULL, NULL},
};

int invoke_cp_cmd(struct user *u, char *cmd, int len)
{
	int i;

	for(i=0; commands[i].name; i++) {
		/* if they don't begin the same, skip... */
		if (strncmp(commands[i].name, cmd, len))
			continue;

		/*
		 * if they aren't the same length, and the next char isn't a
		 * space, skip...
		 */
		if (strnlen(commands[i].name, len) != len &&
		    cmd[strnlen(commands[i].name, len)] != ' ')
			continue;

		return commands[i].f(u, cmd, len);
	}

	return -EINVAL;
}
