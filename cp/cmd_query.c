static int cmd_query(struct user *u, char *cmd, int len)
{
	con_printf(u->con, "got a query! '%s'\n", cmd);
	return 0;
}
