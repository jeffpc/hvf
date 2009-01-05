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
#if 0
	{"GPR", cmd_store_gpr, NULL},
	{"FPR", cmd_store_fpr, NULL},
	{"CR", cmd_store_cr, NULL},
	{"AR", cmd_store_ar, NULL},
#endif
	{"PSW", cmd_store_psw, NULL},
	{NULL, NULL, NULL},
};
