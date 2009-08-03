static int parse_addrspec(u64 *val, u64 *len, char *s)
{
	u64 tmp;
	int parsed = 0;

	tmp = 0;
	while(*s != '\0') {
		if (*s >= '0' && *s <= '9')
			tmp = 16*tmp + (*s - '0');
		else if ((*s >= 'A' && *s <= 'F') ||
			 (*s >= 'a' && *s <= 'f'))
			tmp = 16*tmp + 10 + ((*s & ~0x20) - 'A');
		else if (*s == '.' && parsed)
			break;
		else
			return -EINVAL;

		s++;
		parsed = 1;
	}

	if (len) {
		*len = 0;
		if (*s == '.') {
			s = __extract_hex(s+1, len);
			if (IS_ERR(s))
				parsed = 0;
		}
	}

	*val = tmp;

	return (parsed == 1 ? 0 : -EINVAL);
}

enum display_fmt {
	FMT_NUMERIC = 0,
	FMT_INSTRUCT,
};

static void __display_storage_instruct(struct virt_sys *sys, u64 guest_addr,
				       u64 mlen)
{
	int ret;
	char buf[64];
	int ilen;
	u64 val;
	u64 host_addr;

	u64 end_addr;

	/* walk the page tables to find the real page frame */
	ret = virt2phy_current(guest_addr, &host_addr);
	if (ret) {
		con_printf(sys->con, "DISPLAY: Specified address is not part of "
			   "guest configuration (RC=%d,%d)\n", -EFAULT, ret);
		return;
	}

	if (!mlen)
		mlen = 1;

	end_addr = guest_addr + mlen;

	while(guest_addr < end_addr) {
		/*
		 * FIXME: make sure crossing page-boundary doesn't break
		 * give us garbage
		 */
		ilen = disassm((unsigned char*) host_addr, buf, 64);

		if ((host_addr & PAGE_MASK) >= (PAGE_SIZE-ilen)) {
			con_printf(sys->con, "DISPLAY: Instruction spans page "
				   "boundary - not supported\n");
			break;
		}

		/* load the dword at the address, and shift it to the LSB part */
		val = *((u64*)host_addr) >> 8*(sizeof(u64) - ilen);

		con_printf(sys->con, "R%016llX  %0*llX%*s  %s\n", guest_addr,
			   2*ilen, val,			/* inst hex dump */
			   12-2*ilen, "",		/* spacer */
			   buf);

		host_addr += ilen;
		guest_addr += ilen;
	}
}

static void __display_storage_numeric(struct virt_sys *sys, u64 guest_addr,
				      u64 mlen)
{
	char buf[80];
	char *bp;

	u64 host_addr;
	int this_len;

	int ret;

	/* round down */
	guest_addr &= ~((u64) 0x3);

	/* walk the page tables to find the real page frame */
	ret = virt2phy_current(guest_addr, &host_addr);
	if (ret)
		goto fault;

	if (mlen > 4)
		mlen = (mlen >> 2) + !!(mlen & 0x3);
	else
		mlen = 1;

	while(mlen && !ret) {
		this_len = (mlen > 4) ? 4 : mlen;

		mlen -= this_len;

		bp = buf;
		bp += snprintf(bp, 80, "R%016llX  ", guest_addr);

		while(this_len) {
			bp += snprintf(bp, 80 - (bp - buf), "%08X ",
				       *(u32*)host_addr);

			guest_addr += 4;
			host_addr += 4;
			this_len--;

			/* loop if we're not crossing a page, or if we're done */
			if (((guest_addr & PAGE_MASK) != 0) || !mlen)
				continue;

			/*
			 * We will attempt to walk further along guest
			 * storage, and are about to cross a page boundary,
			 * walk the page tables to find the real page frame
			 */
			ret = virt2phy_current(guest_addr, &host_addr);
			if (ret)
				break;
		}

		con_printf(sys->con, "%s\n", buf);
	}

fault:
	if (ret)
		con_printf(sys->con, "DISPLAY: The address %016llX is not part of "
			   "guest configuration (RC=%d,%d)\n", guest_addr, -EFAULT, ret);
}

/*
 *!!! DISPLAY STORAGE
 *!p                       .-N-.
 *!p >>--DISPLAY--STORAGE--+---+-addr-.----------.---------------------------------><
 *!p                       '-I-'      '-.-length-'
 *!! AUTH G
 *!! PURPOSE
 *! Displays a portion of guest's storage.
 */
static int cmd_display_storage(struct virt_sys *sys, char *cmd, int len)
{
	int ret;
	u64 guest_addr;
	u64 mlen = 0;
	enum display_fmt fmt;

	switch (cmd[0]) {
		case 'N': case 'n':
			/* numeric */
			cmd++;
			fmt = FMT_NUMERIC;
			break;
		case 'I': case 'i':
			/* instruction */
			cmd++;
			fmt = FMT_INSTRUCT;
			break;
		default:
			/* numeric */
			fmt = FMT_NUMERIC;
			break;
	}

	ret = parse_addrspec(&guest_addr, &mlen, cmd);
	if (ret) {
		con_printf(sys->con, "DISPLAY: Invalid addr-spec '%s'\n", cmd);
		return 0;
	}

	if (fmt == FMT_INSTRUCT)
		__display_storage_instruct(sys, guest_addr, mlen);
	else
		__display_storage_numeric(sys, guest_addr, mlen);

	return 0;
}

/*
 *!!! DISPLAY SIECB
 *!p >>--DISPLAY--SIECB------------------------------------------------------------><
 *!! AUTH A
 *!! PURPOSE
 *! Displays hexdump of the guest's SIE control block.
 */
static int cmd_display_siecb(struct virt_sys *sys, char *cmd, int len)
{
	u32 *val;
	int i;

	CP_CMD_AUTH(sys, 'A');

	val = (u32*) &sys->task->cpu->sie_cb;

	for(i=0; i<(sizeof(struct sie_cb)/sizeof(u32)); i+=4)
		con_printf(sys->con, "%03lX  %08X %08X %08X %08X\n",
			   i*sizeof(u32), val[i], val[i+1], val[i+2],
			   val[i+3]);

	return 0;
}

/*
 *!!! DISPLAY GPR
 *!p >>--DISPLAY--GPR--------------------------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Displays the guest's general purpose registers
 */
static int cmd_display_gpr(struct virt_sys *sys, char *cmd, int len)
{
	con_printf(sys->con, "GR  0 = %016llX %016llX\n",
		   sys->task->cpu->regs.gpr[0],
		   sys->task->cpu->regs.gpr[1]);
	con_printf(sys->con, "GR  2 = %016llX %016llX\n",
		   sys->task->cpu->regs.gpr[2],
		   sys->task->cpu->regs.gpr[3]);
	con_printf(sys->con, "GR  4 = %016llX %016llX\n",
		   sys->task->cpu->regs.gpr[4],
		   sys->task->cpu->regs.gpr[5]);
	con_printf(sys->con, "GR  6 = %016llX %016llX\n",
		   sys->task->cpu->regs.gpr[6],
		   sys->task->cpu->regs.gpr[7]);
	con_printf(sys->con, "GR  8 = %016llX %016llX\n",
		   sys->task->cpu->regs.gpr[8],
		   sys->task->cpu->regs.gpr[9]);
	con_printf(sys->con, "GR 10 = %016llX %016llX\n",
		   sys->task->cpu->regs.gpr[10],
		   sys->task->cpu->regs.gpr[11]);
	con_printf(sys->con, "GR 12 = %016llX %016llX\n",
		   sys->task->cpu->regs.gpr[12],
		   sys->task->cpu->regs.gpr[13]);
	con_printf(sys->con, "GR 14 = %016llX %016llX\n",
		   sys->task->cpu->regs.gpr[14],
		   sys->task->cpu->regs.gpr[15]);
	return 0;
}

/*
 *!!! DISPLAY FPCR
 *!p >>--DISPLAY--FPCR-------------------------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Displays the guest's floating point control register
 */
static int cmd_display_fpcr(struct virt_sys *sys, char *cmd, int len)
{
	con_printf(sys->con, "FPCR  = %08X\n", sys->task->cpu->regs.fpcr);
	return 0;
}

/*
 *!!! DISPLAY FPR
 *!p >>--DISPLAY--FPR--------------------------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Displays the guest's floating point registers
 */
static int cmd_display_fpr(struct virt_sys *sys, char *cmd, int len)
{
	con_printf(sys->con, "FR  0 = %016llX %016llX\n",
		   sys->task->cpu->regs.fpr[0],
		   sys->task->cpu->regs.fpr[1]);
	con_printf(sys->con, "FR  2 = %016llX %016llX\n",
		   sys->task->cpu->regs.fpr[2],
		   sys->task->cpu->regs.fpr[3]);
	con_printf(sys->con, "FR  4 = %016llX %016llX\n",
		   sys->task->cpu->regs.fpr[4],
		   sys->task->cpu->regs.fpr[5]);
	con_printf(sys->con, "FR  6 = %016llX %016llX\n",
		   sys->task->cpu->regs.fpr[6],
		   sys->task->cpu->regs.fpr[7]);
	con_printf(sys->con, "FR  8 = %016llX %016llX\n",
		   sys->task->cpu->regs.fpr[8],
		   sys->task->cpu->regs.fpr[9]);
	con_printf(sys->con, "FR 10 = %016llX %016llX\n",
		   sys->task->cpu->regs.fpr[10],
		   sys->task->cpu->regs.fpr[11]);
	con_printf(sys->con, "FR 12 = %016llX %016llX\n",
		   sys->task->cpu->regs.fpr[12],
		   sys->task->cpu->regs.fpr[13]);
	con_printf(sys->con, "FR 14 = %016llX %016llX\n",
		   sys->task->cpu->regs.fpr[14],
		   sys->task->cpu->regs.fpr[15]);
	return 0;
}

/*
 *!!! DISPLAY CR
 *!p >>--DISPLAY--CR---------------------------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Displays the guest's control registers
 */
static int cmd_display_cr(struct virt_sys *sys, char *cmd, int len)
{
	con_printf(sys->con, "CR  0 = %016llX %016llX\n",
		   sys->task->cpu->sie_cb.gcr[0],
		   sys->task->cpu->sie_cb.gcr[1]);
	con_printf(sys->con, "CR  2 = %016llX %016llX\n",
		   sys->task->cpu->sie_cb.gcr[2],
		   sys->task->cpu->sie_cb.gcr[3]);
	con_printf(sys->con, "CR  4 = %016llX %016llX\n",
		   sys->task->cpu->sie_cb.gcr[4],
		   sys->task->cpu->sie_cb.gcr[5]);
	con_printf(sys->con, "CR  6 = %016llX %016llX\n",
		   sys->task->cpu->sie_cb.gcr[6],
		   sys->task->cpu->sie_cb.gcr[7]);
	con_printf(sys->con, "CR  8 = %016llX %016llX\n",
		   sys->task->cpu->sie_cb.gcr[8],
		   sys->task->cpu->sie_cb.gcr[9]);
	con_printf(sys->con, "CR 10 = %016llX %016llX\n",
		   sys->task->cpu->sie_cb.gcr[10],
		   sys->task->cpu->sie_cb.gcr[11]);
	con_printf(sys->con, "CR 12 = %016llX %016llX\n",
		   sys->task->cpu->sie_cb.gcr[12],
		   sys->task->cpu->sie_cb.gcr[13]);
	con_printf(sys->con, "CR 14 = %016llX %016llX\n",
		   sys->task->cpu->sie_cb.gcr[14],
		   sys->task->cpu->sie_cb.gcr[15]);
	return 0;
}

/*
 *!!! DISPLAY AR
 *!p >>--DISPLAY--AR---------------------------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Displays the guest's access registers
 */
static int cmd_display_ar(struct virt_sys *sys, char *cmd, int len)
{
	con_printf(sys->con, "AR  0 = %08X %08X\n",
		   sys->task->cpu->regs.ar[0],
		   sys->task->cpu->regs.ar[1]);
	con_printf(sys->con, "AR  2 = %08X %08X\n",
		   sys->task->cpu->regs.ar[2],
		   sys->task->cpu->regs.ar[3]);
	con_printf(sys->con, "AR  4 = %08X %08X\n",
		   sys->task->cpu->regs.ar[4],
		   sys->task->cpu->regs.ar[5]);
	con_printf(sys->con, "AR  6 = %08X %08X\n",
		   sys->task->cpu->regs.ar[6],
		   sys->task->cpu->regs.ar[7]);
	con_printf(sys->con, "AR  8 = %08X %08X\n",
		   sys->task->cpu->regs.ar[8],
		   sys->task->cpu->regs.ar[9]);
	con_printf(sys->con, "AR 10 = %08X %08X\n",
		   sys->task->cpu->regs.ar[10],
		   sys->task->cpu->regs.ar[11]);
	con_printf(sys->con, "AR 12 = %08X %08X\n",
		   sys->task->cpu->regs.ar[12],
		   sys->task->cpu->regs.ar[13]);
	con_printf(sys->con, "AR 14 = %08X %08X\n",
		   sys->task->cpu->regs.ar[14],
		   sys->task->cpu->regs.ar[15]);
	return 0;
}

/*
 *!!! DISPLAY PSW
 *!p >>--DISPLAY--PSW--------------------------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Displays the guest's PSW
 */
static int cmd_display_psw(struct virt_sys *sys, char *cmd, int len)
{
	u32 *ptr = (u32*) &sys->task->cpu->sie_cb.gpsw;

	con_printf(sys->con, "PSW = %08X %08X %08X %08X\n",
		   ptr[0], ptr[1], ptr[2], ptr[3]);
	return 0;
}

static void __do_display_schib(struct console *con, struct virt_device *vdev)
{
	con_printf(con, "%05X %04X %08X   %d  %02X %02X  %02X  %02X %02X "
		        "---- %02X %02X %02X%02X%02X%02X %02X%02X%02X%02X\n",
		   vdev->sch, vdev->pmcw.dev_num, vdev->pmcw.interrupt_param,
		   vdev->pmcw.isc, ((vdev->pmcw.e  << 7) |
				    (vdev->pmcw.lm << 5) |
				    (vdev->pmcw.mm << 3) |
				    (vdev->pmcw.d  << 2) |
				    (vdev->pmcw.t  << 1) |
				    (vdev->pmcw.v)),
		   vdev->pmcw.lpm, vdev->pmcw.pnom, vdev->pmcw.lpum,
		   vdev->pmcw.pim,
		   /* MBI */
		   vdev->pmcw.pom, vdev->pmcw.pam,
		   vdev->pmcw.chpid[0], vdev->pmcw.chpid[1],
		   vdev->pmcw.chpid[2], vdev->pmcw.chpid[3],
		   vdev->pmcw.chpid[4], vdev->pmcw.chpid[5],
		   vdev->pmcw.chpid[6], vdev->pmcw.chpid[7]);
}

/*
 *!!! DISPLAY SCHIB
 *!p >>--DISPLAY--SCHIB--.-ALL---.-------------------------------------------------><
 *!p                     '-schib-'
 *!! AUTH G
 *!! PURPOSE
 *! Displays the guest's subchannel control block information
 */
static int cmd_display_schib(struct virt_sys *sys, char *cmd, int len)
{
	struct virt_device *vdev;
	u64 sch;
	int all;

	if (strcasecmp(cmd, "ALL")) {
		cmd = __extract_hex(cmd, &sch);
		if (IS_ERR(cmd))
			return PTR_ERR(cmd);

		/* sch number must be: X'0001____' */
		if ((sch & 0xffff0000) != 0x00010000)
			return -EINVAL;

		all = 0;
	} else
		all = 1;

	/* find the virtual device */

	list_for_each_entry(vdev, &sys->virt_devs, devices) {
		if ((vdev->sch == (u32) sch) || all) {
			if (!all || all == 1) {
				con_printf(sys->con, "SCHIB DEV  INT-PARM ISC FLG LP "
					   "PNO LPU PI MBI  PO PA CHPID0-3 CHPID4-7\n");
				all = (all ? 2 : 0);
			}

			__do_display_schib(sys->con, vdev);

			if (!all)
				break;
		}
	}

	return 0;
}

static struct cpcmd cmd_tbl_display[] = {
	{"AR",		cmd_display_ar,		NULL},
	{"A",		cmd_display_ar,		NULL},

	{"CR",		cmd_display_cr,		NULL},
	{"C",		cmd_display_cr,		NULL},

	{"FPCR",	cmd_display_fpcr,	NULL},

	{"FPR",		cmd_display_fpr,	NULL},
	{"FP",		cmd_display_fpr,	NULL},
	{"F",		cmd_display_fpr,	NULL},

	{"GPR",		cmd_display_gpr,	NULL},
	{"GP",		cmd_display_gpr,	NULL},
	{"G",		cmd_display_gpr,	NULL},

	{"PSW",		cmd_display_psw,	NULL},

	{"SCHIB",	cmd_display_schib,	NULL},

	{"SIECB",	cmd_display_siecb,	NULL},

	{"STORAGE",	cmd_display_storage,	NULL},
	{"STORAG",	cmd_display_storage,	NULL},
	{"STORA",	cmd_display_storage,	NULL},
	{"STOR",	cmd_display_storage,	NULL},
	{"STO",		cmd_display_storage,	NULL},
	{"",		NULL,			NULL},
};
