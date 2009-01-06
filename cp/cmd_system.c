static int cmd_ipl(struct user *u, char *cmd, int len)
{
	con_printf(u->con, "IPL is not yet supported\n");

	return 0;
}

static int cmd_system(struct user *u, char *cmd, int len)
{
	if (!strcasecmp(cmd, "CLEAR")) {
		guest_system_reset_clear(u);
		con_printf(u->con, "STORAGE CLEARED - SYSTEM RESET\n");
	} else if (!strcasecmp(cmd, "RESET")) {
		guest_system_reset_normal(u);
		con_printf(u->con, "SYSTEM RESET\n");
	} else if (!strcasecmp(cmd, "RESTART")) {
		con_printf(u->con, "SYSTEM RESTART is not yet supported\n");
	} else if (!strcasecmp(cmd, "STORE")) {
		con_printf(u->con, "SYSTEM STORE is not yet supported\n");
	} else
		con_printf(u->con, "SYSTEM: Unknown variable '%s'\n", cmd);

	return 0;
}
