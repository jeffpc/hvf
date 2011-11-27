/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

/*
 *!!! STORE STORAGE
 *!! SYNTAX
 *! \cbstart
 *! \tok{\sc STOre} \tok{\sc STOrage} <address> <value>
 *! \cbend
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Sets a word in guest's storage at address to value
 */
static int cmd_store_storage(struct virt_sys *sys, char *cmd, int len)
{
	int ret;
	u64 guest_addr, host_addr;
	u64 val = 0;

	SHELL_CMD_AUTH(sys, G);

	cmd = parse_addrspec(&guest_addr, NULL, cmd);
	if (IS_ERR(cmd)) {
		con_printf(sys->con, "STORE: Invalid addr-spec\n");
		return PTR_ERR(cmd);
	}

	/* consume any extra whitespace */
	cmd = __consume_ws(cmd);

	/* get the value */
	cmd = __extract_hex(cmd, &val);
	if (IS_ERR(cmd))
		return PTR_ERR(cmd);

	/*
	 * round down to the nearest word
	 */
	guest_addr &= ~((u64) 3ULL);

	/* walk the page tables to find the real page frame */
	ret = virt2phy_current(guest_addr, &host_addr);
	if (ret) {
		con_printf(sys->con, "STORE: Specified address is not part of "
			   "guest configuration\n");
		return ret;
	}

	*((u32*) host_addr) = (u32) val;

	con_printf(sys->con, "Store complete.\n");

	return 0;
}

/*
 *!!! STORE GPR
 *!! SYNTAX
 *! \tok{\sc STOre} \tok{\sc Gpr} <gpr> <value>
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Sets a guest's general purpose register to the specified value
 */
static int cmd_store_gpr(struct virt_sys *sys, char *cmd, int len)
{
	u64 *ptr = (u64*) &sys->task->cpu->regs.gpr;
	u64 val, gpr;

	SHELL_CMD_AUTH(sys, G);

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
 *!! SYNTAX
 *! \tok{\sc STOre} \tok{\sc Fpr} <fpr> <value>
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Sets a guest's floating point register to the specified value
 */
static int cmd_store_fpr(struct virt_sys *sys, char *cmd, int len)
{
	u64 *ptr = (u64*) &sys->task->cpu->regs.fpr;
	u64 val, fpr;

	SHELL_CMD_AUTH(sys, G);

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
 *!! SYNTAX
 *! \tok{\sc STOre} \tok{\sc FPCR} <value>
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Sets a guest's floating point control register to the specified value
 */
static int cmd_store_fpcr(struct virt_sys *sys, char *cmd, int len)
{
	u64 val;

	SHELL_CMD_AUTH(sys, G);

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
 *!! SYNTAX
 *! \tok{\sc STOre} \tok{\sc Cr} <cr> <value>
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Sets a guest's control register to the specified value
 */
static int cmd_store_cr(struct virt_sys *sys, char *cmd, int len)
{
	u64 *ptr = (u64*) &sys->task->cpu->sie_cb.gcr;
	u64 val, cr;

	SHELL_CMD_AUTH(sys, G);

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
 *!! SYNTAX
 *! \tok{\sc STOre} \tok{\sc Ar} <ar> <value>
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Sets a guest's access register to the specified value
 */
static int cmd_store_ar(struct virt_sys *sys, char *cmd, int len)
{
	u32 *ptr = (u32*) &sys->task->cpu->regs.ar;
	u64 val, ar;

	SHELL_CMD_AUTH(sys, G);

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
 *!! SYNTAX
 *! \cbstart
 *! \tok{\sc STOre} \tok{\sc PSW}
 *!     \begin{stack} \\
 *!       \begin{stack} \\
 *!         \begin{stack} \\ <hexword1>
 *!         \end{stack} <hexword2>
 *!       \end{stack} <hexword3>
 *!     \end{stack}
 *! <hexword4>
 *! \cbend
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! \cbstart
 *! Alters all or a part of the PSW.
 *! \cbend
 *!! OPERANDS
 *! \cbstart
 *! \item[hexword...]
 *!   \textbf{For an ESA/390 guest:}
 *!
 *!   Alters all or part of the PSW with the data specified in hexword3
 *!   and hexword4.  If only hexword4 is specified, it is stored to PSW bits
 *!   32-63.  If hexword3 and hexword4 are specified, hexword3 is stored to
 *!   PSW bits 0-31, and hexword4 to PSW bits 32-63.
 *!
 *!   If more than two values are specified, the PSW remains unchanged and an
 *!   message indicating an error is printed.
 *!
 *!   \textbf{For a z/Architecture guest:}
 *!
 *!   If only one hexword4 is specified, PSW bits 96-127 are set to it.
 *!
 *!   If hexword3 and hexword4 are specified, PSW bits 64-95 are set to
 *!   hexword3, and bits 96-127 are set to hexword4.
 *!
 *!   If hexword2, hexword3, and hexword4 are specified, PSW bits 32-63 are
 *!   set to hexword2, bits 64-95 are set to hexword3, and bits 96-127 are
 *!   set to hexword4.
 *!
 *!   If hexword1, hexword2, hexword3, and hexword4 are specified, PSW bits
 *!   0-31 are set to hexword1, bits 32-63 are set to hexword2, bits 64-95
 *!   are set to hexword3, and bits 96-127 are set to hexword4.
 *! \cbend
 *!! SDNAREPO
 */
static int cmd_store_psw(struct virt_sys *sys, char *cmd, int len)
{
	u32 *ptr = (u32*) &sys->task->cpu->sie_cb.gpsw;

	u64 new_words[4] = {0, 0, 0, 0};
	int cnt;

	SHELL_CMD_AUTH(sys, G);

	for (cnt=0; cnt<4; cnt++) {
		cmd = __extract_hex(cmd, &new_words[cnt]);
		if (IS_ERR(cmd))
			break;

		cmd = __consume_ws(cmd);
	}

	if (!cnt)
		return -EINVAL;

	if (!VCPU_ZARCH(sys->task->cpu)) {
		/* ESA/390 mode */
		if (cnt > 2) {
			con_printf(sys->con, "STORE: ERROR ESA/390 PSW USES ONLY TWO WORDS\n");
			return -EINVAL;
		}

		/*
		 * This is a simple "hack" to make the stores go into the
		 * right place.  If he had a single word, we want it to go
		 * the the second word of the 16-byte PSW, not the 4th.
		 * Similarly, if we had two words, we want them to go into
		 * the first and second words of the 16-byte PSW.
		 */
		cnt += 2;
	}

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
	{"AR",		cmd_store_ar,		NULL},
	{"A",		cmd_store_ar,		NULL},

	{"CR",		cmd_store_cr,		NULL},
	{"C",		cmd_store_cr,		NULL},

	{"FPCR",	cmd_store_fpcr,		NULL},

	{"FPR",		cmd_store_fpr,		NULL},
	{"FP",		cmd_store_fpr,		NULL},
	{"F",		cmd_store_fpr,		NULL},

	{"GPR",		cmd_store_gpr,		NULL},
	{"GP",		cmd_store_gpr,		NULL},
	{"G",		cmd_store_gpr,		NULL},

	{"PSW",		cmd_store_psw,		NULL},

	{"STORAGE",	cmd_store_storage,	NULL},
	{"STORAG",	cmd_store_storage,	NULL},
	{"STORA",	cmd_store_storage,	NULL},
	{"STOR",	cmd_store_storage,	NULL},
	{"STO",		cmd_store_storage,	NULL},
	{"",		NULL,			NULL},
};
