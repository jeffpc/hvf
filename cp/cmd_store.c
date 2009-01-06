static int cmd_store_storage(struct user *u, char *cmd, int len)
{
	int ret;
	u64 guest_addr, host_addr;
	u64 val = 0;

	/* get the value */
	cmd = __extract_hex(cmd, &val);
	if (IS_ERR(cmd))
		return PTR_ERR(cmd);

	/* consume any extra whitespace */
	cmd = __consume_ws(cmd);

	ret = parse_addrspec(&guest_addr, cmd, len);
	if (ret) {
		con_printf(u->con, "DISPLAY: Invalid addr-spec '%s'\n", cmd);
		return ret;
	}

	/*
	 * round down to the nearest word
	 */
	guest_addr &= ~((u64) 3ULL);

	/* walk the page tables to find the real page frame */
	ret = virt2phy(&current->guest->as, guest_addr, &host_addr);
	if (ret) {
		con_printf(u->con, "DISPLAY: Specified address is not part of "
			   "guest configuration\n");
		return ret;
	}

	*((u32*) host_addr) = (u32) val;

	con_printf(u->con, "Store complete.\n");

	return 0;
}

static int cmd_store_gpr(struct user *u, char *cmd, int len)
{
	u64 *ptr = (u64*) &current->guest->regs.gpr;
	u64 val, gpr;

	cmd = __extract_hex(cmd, &gpr);
	if (IS_ERR(cmd))
		return PTR_ERR(cmd);
	if (gpr > 15)
		return -EINVAL;

	cmd = __consume_ws(cmd);

	cmd = __extract_hex(cmd, &val);
	if (IS_ERR(cmd))
		return PTR_ERR(cmd);

	ptr[gpr] = val;

	con_printf(u->con, "Store complete.\n");

	return 0;
}

static int cmd_store_fpr(struct user *u, char *cmd, int len)
{
	u64 *ptr = (u64*) &current->guest->regs.fpr;
	u64 val, fpr;

	cmd = __extract_hex(cmd, &fpr);
	if (IS_ERR(cmd))
		return PTR_ERR(cmd);
	if (fpr > 15)
		return -EINVAL;

	cmd = __consume_ws(cmd);

	cmd = __extract_hex(cmd, &val);
	if (IS_ERR(cmd))
		return PTR_ERR(cmd);

	ptr[fpr] = val;

	con_printf(u->con, "Store complete.\n");

	return 0;
}

static int cmd_store_fpcr(struct user *u, char *cmd, int len)
{
	u64 val;

	cmd = __extract_hex(cmd, &val);
	if (IS_ERR(cmd))
		return PTR_ERR(cmd);
	if (val > 0xffffffffULL)
		return -EINVAL;

	current->guest->regs.fpcr = (u32) val;

	con_printf(u->con, "Store complete.\n");

	return 0;
}

static int cmd_store_cr(struct user *u, char *cmd, int len)
{
	u64 *ptr = (u64*) &current->guest->sie_cb.gcr;
	u64 val, cr;

	cmd = __extract_hex(cmd, &cr);
	if (IS_ERR(cmd))
		return PTR_ERR(cmd);
	if (cr > 15)
		return -EINVAL;

	cmd = __consume_ws(cmd);

	cmd = __extract_hex(cmd, &val);
	if (IS_ERR(cmd))
		return PTR_ERR(cmd);

	ptr[cr] = val;

	con_printf(u->con, "Store complete.\n");

	return 0;
}

static int cmd_store_ar(struct user *u, char *cmd, int len)
{
	u32 *ptr = (u32*) &current->guest->regs.ar;
	u64 val, ar;

	cmd = __extract_hex(cmd, &ar);
	if (IS_ERR(cmd))
		return PTR_ERR(cmd);
	if (ar > 15)
		return -EINVAL;

	cmd = __consume_ws(cmd);

	cmd = __extract_hex(cmd, &val);
	if (IS_ERR(cmd))
		return PTR_ERR(cmd);
	if (val > 0xffffffffULL)
		return -EINVAL;

	ptr[ar] = val;

	con_printf(u->con, "Store complete.\n");

	return 0;
}

static int cmd_store_psw(struct user *u, char *cmd, int len)
{
	u32 *ptr = (u32*) &current->guest->sie_cb.gpsw;

	u64 new_words[4] = {0, 0, 0, 0};
	int cnt;

	for (cnt=0; cnt<4; cnt++) {
		cmd = __extract_hex(cmd, &new_words[cnt]);
		if (IS_ERR(cmd))
			break;

		cmd = __consume_ws(cmd);
	}

	if (!cnt)
		return -EINVAL;

	switch(cnt) {
		case 4:
			ptr[0] = (u32) new_words[0];
			ptr[1] = (u32) new_words[1];
			ptr[2] = (u32) new_words[2];
			ptr[3] = (u32) new_words[3];
			break;
		case 3:
			ptr[1] = (u32) new_words[0];
			ptr[2] = (u32) new_words[1];
			ptr[3] = (u32) new_words[2];
			break;
		case 2:
			ptr[2] = (u32) new_words[0];
			ptr[3] = (u32) new_words[1];
			break;
		case 1:
			ptr[3] = (u32) new_words[0];
			break;
		case 0:
			return -EINVAL;
	}

	con_printf(u->con, "Store complete.\n");
	return 0;
}

static struct cpcmd cmd_tbl_store[] = {
	{"STORAGE", cmd_store_storage, NULL},
	{"GPR", cmd_store_gpr, NULL},
	{"FPCR", cmd_store_fpcr, NULL},
	{"FPR", cmd_store_fpr, NULL},
	{"CR", cmd_store_cr, NULL},
	{"AR", cmd_store_ar, NULL},
	{"PSW", cmd_store_psw, NULL},
	{NULL, NULL, NULL},
};
