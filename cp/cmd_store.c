static int cmd_store_storage(struct user *u, char *cmd, int len)
{
	int ret;
	u64 guest_addr, host_addr;
	u32 val = 0, tmp=0;
	int ws_only;

	/* get the value */
	for (ws_only=1; cmd && *cmd != '\0' && (*cmd != ' ' || ws_only); cmd++) {
		if (*cmd >= '0' && *cmd <= '9')
			tmp = *cmd - '0';
		else if (*cmd >= 'A' && *cmd <= 'F')
			tmp = *cmd - 'A' + 10;
		else if (*cmd >= 'a' && *cmd <= 'f')
			tmp = *cmd - 'a' + 10;
		else if (*cmd == ' ' || *cmd == '\t') {
			continue;
		}

		val = val*16 + tmp;
		ws_only = 0;
	}

	if (ws_only)
		return -EINVAL;

	/* consume any extra whitespace */
	while(*cmd == ' ' || *cmd == '\t')
		cmd++;

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

	*((u32*) host_addr) = val;

	con_printf(u->con, "Store complete.\n");

	return 0;
}

static int cmd_store_psw(struct user *u, char *cmd, int len)
{
	u32 *ptr = (u32*) &current->guest->sie_cb.gpsw;

	u32 new_words[4] = {0, 0, 0, 0};
	u32 tmp = 0;
	int cnt, last_ws;

	for (cnt=0, last_ws=1; cmd && *cmd != '\0'; cmd++) {
		if (*cmd >= '0' && *cmd <= '9')
			tmp = *cmd - '0';
		else if (*cmd >= 'A' && *cmd <= 'F')
			tmp = *cmd - 'A' + 10;
		else if (*cmd >= 'a' && *cmd <= 'f')
			tmp = *cmd - 'a' + 10;
		else if (*cmd == ' ' || *cmd == '\t') {
			if (!last_ws)
				cnt++;
			last_ws = 1;
			continue;
		}

		new_words[cnt] = new_words[cnt]*16 + tmp;
		last_ws = 0;
	}

	if (!last_ws)
		cnt++;

	switch(cnt) {
		case 4:
			ptr[0] = new_words[0];
			ptr[1] = new_words[1];
			ptr[2] = new_words[2];
			ptr[3] = new_words[3];
			break;
		case 3:
			ptr[1] = new_words[0];
			ptr[2] = new_words[1];
			ptr[3] = new_words[2];
			break;
		case 2:
			ptr[2] = new_words[0];
			ptr[3] = new_words[1];
			break;
		case 1:
			ptr[3] = new_words[0];
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
