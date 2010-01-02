/*
 *!!! LOGON
 *!! SYNTAX
 *! \tok{\sc LOGON} <userid>
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Log on to a virtual machine.
 */
static int cmd_logon(struct virt_sys *data, char *cmd, int len)
{
	struct console *con = (struct console*) data; /* BEWARE */
	struct user *u;

	u = find_user_by_id(cmd);
	if (IS_ERR(u)) {
		con_printf(con, "INVALID USERID\n");
		return 0;
	}

	spawn_user_shell(con, u);
	con_printf(oper_con, "%s %04X LOGON AS %-8s\n",
		   type2name(con->dev->type), con->dev->ccuu, u->userid);

	return 0;
}

static int cmd_logon_fail(struct virt_sys *sys, char *cmd, int len)
{
	con_printf(sys->con, "ALREADY LOGGED ON\n");
	return 0;
}
