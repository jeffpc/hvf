/*
 *!!! STORE STORAGE
 *!p >>--STORE--STORAGE--value--address--------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Sets a word in guest's storage at address to value
 */
static int cmd_store_storage(struct virt_sys *sys, char *cmd, int len)
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

	ret = parse_addrspec(&guest_addr, NULL, cmd);
	if (ret) {
		con_printf(sys->con, "DISPLAY: Invalid addr-spec '%s'\n", cmd);
		return ret;
	}

	/*
	 * round down to the nearest word
	 */
	guest_addr &= ~((u64) 3ULL);

	/* walk the page tables to find the real page frame */
	ret = virt2phy_current(guest_addr, &host_addr);
	if (ret) {
		con_printf(sys->con, "DISPLAY: Specified address is not part of "
			   "guest configuration\n");
		return ret;
	}

	*((u32*) host_addr) = (u32) val;

	con_printf(sys->con, "Store complete.\n");

	return 0;
}

/*
 *!!! STORE GPR
 *!p >>--STORE--GPR--gpr--value----------------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Sets a guest's general purpose register to the specified value
 */
static int cmd_store_gpr(struct virt_sys *sys, char *cmd, int len)
{
	u64 *ptr = (u64*) &sys->task->cpu->regs.gpr;
	u64 val, gpr;

	cmd = __extract_dec(cmd, &gpr);
	if (IS_ERR(cmd))
		return PTR_ERR(cmd);
	if (gpr > 15)
		return -EINVAL;

	cmd = __consume_ws(cmd);

	cmd = __extract_hex(cmd, &val);
	if (IS_ERR(cmd))
		return PTR_ERR(cmd);

	ptr[gpr] = val;

	con_printf(sys->con, "Store complete.\n");

	return 0;
}

/*
 *!!! STORE FPR
 *!p >>--STORE--FPR--fpr--value----------------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Sets a guest's floating point register to the specified value
 */
static int cmd_store_fpr(struct virt_sys *sys, char *cmd, int len)
{
	u64 *ptr = (u64*) &sys->task->cpu->regs.fpr;
	u64 val, fpr;

	cmd = __extract_dec(cmd, &fpr);
	if (IS_ERR(cmd))
		return PTR_ERR(cmd);
	if (fpr > 15)
		return -EINVAL;

	cmd = __consume_ws(cmd);

	cmd = __extract_hex(cmd, &val);
	if (IS_ERR(cmd))
		return PTR_ERR(cmd);

	ptr[fpr] = val;

	con_printf(sys->con, "Store complete.\n");

	return 0;
}

/*
 *!!! STORE FPCR
 *!p >>--STORE--FPCR--value--------------------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Sets a guest's floating point control register to the specified value
 */
static int cmd_store_fpcr(struct virt_sys *sys, char *cmd, int len)
{
	u64 val;

	cmd = __extract_hex(cmd, &val);
	if (IS_ERR(cmd))
		return PTR_ERR(cmd);
	if (val > 0xffffffffULL)
		return -EINVAL;

	sys->task->cpu->regs.fpcr = (u32) val;

	con_printf(sys->con, "Store complete.\n");

	return 0;
}

/*
 *!!! STORE CR
 *!p >>--STORE--CR--cr--value------------------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Sets a guest's control register to the specified value
 */
static int cmd_store_cr(struct virt_sys *sys, char *cmd, int len)
{
	u64 *ptr = (u64*) &sys->task->cpu->sie_cb.gcr;
	u64 val, cr;

	cmd = __extract_dec(cmd, &cr);
	if (IS_ERR(cmd))
		return PTR_ERR(cmd);
	if (cr > 15)
		return -EINVAL;

	cmd = __consume_ws(cmd);

	cmd = __extract_hex(cmd, &val);
	if (IS_ERR(cmd))
		return PTR_ERR(cmd);

	ptr[cr] = val;

	con_printf(sys->con, "Store complete.\n");

	return 0;
}

/*
 *!!! STORE AR
 *!p >>--STORE--AR--ar--value------------------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Sets a guest's access register to the specified value
 */
static int cmd_store_ar(struct virt_sys *sys, char *cmd, int len)
{
	u32 *ptr = (u32*) &sys->task->cpu->regs.ar;
	u64 val, ar;

	cmd = __extract_dec(cmd, &ar);
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

	con_printf(sys->con, "Store complete.\n");

	return 0;
}

/*
 *!!! STORE PSW
 *!p >>--STORE--PSW--val1-.--------------------------.-----------------------------><
 *!p                      '-val2--.----------------.-'
 *!p                              '-val3--.------.-'
 *!p                                      '-val4-'
 *!! AUTH G
 *!! PURPOSE
 *! Sets a guest's PSW to the specified value
 *!
 *! If only one val1 is specified, PSW bits 96-127 are set to it.
 *!
 *! If val1 and val2 are specified, PSW bits 64-95 are set to val1, and bits
 *! 96-127 are set to val2.
 *!
 *! If val1, val2, and val3 are specified, PSW bits 32-63 are set to val1,
 *! bits 64-95 are set to val2, and bits 96-127 are set to val3.
 *!
 *! If val1, val2, val3, and val4 are specified, PSW bits 0-31 are set to
 *! val1, bits 32-63 are set to val2, bits 64-95 are set to val3, and
 *! bits 96-127 are set to val4.
 */
static int cmd_store_psw(struct virt_sys *sys, char *cmd, int len)
{
	u32 *ptr = (u32*) &sys->task->cpu->sie_cb.gpsw;

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

	con_printf(sys->con, "Store complete.\n");
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
