static int cmd_query(struct user *u, char *cmd, int len)
{
	if (!strcasecmp(cmd, "CPLEVEL")) {
		con_printf(u->con, "HVF version " VERSION "\n");
		con_printf(u->con, "IPL at %02d:%02d:%02d UTC %04d-%02d-%02d\n",
			   ipltime.th, ipltime.tm, ipltime.ts, ipltime.dy,
			   ipltime.dm, ipltime.dd);
	} else
		con_printf(u->con, "QUERY: Unknown variable '%s'\n", cmd);

	return 0;
}
