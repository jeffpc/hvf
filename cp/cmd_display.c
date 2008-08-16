static int cmd_display_storage(struct user *u, char *cmd, int len)
{
	con_printf(u->con, "display storage '%s' (%d)\n", cmd, len);
	return 0;
}

static struct cpcmd cmd_tbl_display[] = {
	{"STORAGE", cmd_display_storage, NULL},
#if 0
	{"GPR", cmd_display_gpr, NULL},
	{"FPR", cmd_display_fpr, NULL},
	{"CR", cmd_display_cr, NULL},
	{"AR", cmd_display_ar, NULL},
	{"PSW", cmd_display_psw, NULL},
#endif
	{NULL, NULL, NULL},
};
