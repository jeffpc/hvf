/*
 *!!! SET NOTS
 *!p >>--SET--NOTS-----------------------------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Disable the console timestamp printing
 */
static int cmd_set_nots(struct virt_sys *sys, char *cmd, int len)
{
	sys->print_ts = 0;
	return 0;
}

/*
 *!!! SET TS
 *!p >>--SET--TS-------------------------------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Enable the console timestamp printing
 */
static int cmd_set_ts(struct virt_sys *sys, char *cmd, int len)
{
	sys->print_ts = 1;
	return 0;
}

static struct cpcmd cmd_tbl_set[] = {
	{"NOTS",	cmd_set_nots,		NULL},
	{"TS",		cmd_set_ts,		NULL},
	{"",		NULL,			NULL},
};
