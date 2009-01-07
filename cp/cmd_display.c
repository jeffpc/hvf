static int parse_addrspec(u64 *ret, char *s, int len)
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
		else
			return -EINVAL;

		s++;
		parsed = 1;
	}

	*ret = tmp;
	return (parsed == 1 ? 0 : -EINVAL);
}

/*
 * display a word of guest storage
 */
static int cmd_display_storage(struct virt_sys *sys, char *cmd, int len)
{
	int ret;
	u64 guest_addr, host_addr, val;
	int mlen = 0;
	int dis = 0;

	switch (cmd[0]) {
		case 'C': case 'c':
			/* char */
			mlen = 1; cmd++; len--; break;
		case 'H': case 'h':
			/* half-word */
			mlen = 2; cmd++; len--; break;
		case 'W': case 'w':
			/* word */
			mlen = 4; cmd++; len--; break;
		case 'D': case 'd':
			/* double-word */
			mlen = 8; cmd++; len--; break;
		case 'I': case 'i':
			/* instruction */
			dis = 1;
			cmd++; len--;
			break;
		default:
			con_printf(sys->con, "DISPLAY: Invalid length/type '%s'\n",
				   cmd);
			return -EINVAL;
	}

	ret = parse_addrspec(&guest_addr, cmd, len);
	if (ret) {
		con_printf(sys->con, "DISPLAY: Invalid addr-spec '%s'\n", cmd);
		return ret;
	}

	/*
	 * round down to the nearest word/dword/etc - only if not
	 * disassembling
	 */
	if (!dis)
		guest_addr &= ~((u64) mlen-1);

	/* walk the page tables to find the real page frame */
	ret = virt2phy(&sys->as, guest_addr, &host_addr);
	if (ret) {
		con_printf(sys->con, "DISPLAY: Specified address is not part of "
			   "guest configuration\n");
		return ret;
	}

	if (dis) {
		char buf[64];

		/*
		 * FIXME: make sure crossing page-boundary doesn't break
		 * give us garbage
		 */
		mlen = disassm((unsigned char*) host_addr, buf, 64);

		if ((mlen > 2) && ((host_addr & (PAGE_SIZE-1)) >= (PAGE_SIZE-2))) {
			con_printf(sys->con, "DISPLAY: Instruction spans page "
				   "boundary - not supported\n");
			return 0;
		}

		/* load the dword at the address, and shift it to the LSB part */
		val = *((u64*)host_addr) >> 8*(sizeof(u64) - mlen);

		con_printf(sys->con, "R%016llX  %0*llX%*s  %s\n", guest_addr,
			   2*mlen, val,			/* inst hex dump */
			   12-2*mlen, "",		/* spacer */
			   buf);
	} else {
		/* load the dword at the address, and shift it to the LSB part */
		val = *((u64*)host_addr) >> 8*(sizeof(u64) - mlen);

		/*
		 * this is a clever trick to have one format string and have it
		 * print (almost arbitrary amount of data); we give it a length (in
		 * digits) of the result, and always give it a u64 value
		 */
		if (mlen < 8)
			con_printf(sys->con, "R%016llX  %0*llX\n", guest_addr,
				   mlen << 1, val);
		else if (mlen == 8)
			con_printf(sys->con, "R%016llX  %08llX %08llX\n", guest_addr,
				   val >> 32, val & 0xffffffffULL);
		else
			con_printf(sys->con, "DISPLAY: unhandled display length\n");
	}

	return 0;
}

static int cmd_display_siecb(struct virt_sys *sys, char *cmd, int len)
{
	u32 *val;
	int i;

	val = (u32*) &sys->task->cpu->sie_cb;

	for(i=0; i<(sizeof(struct sie_cb)/sizeof(u32)); i+=4)
		con_printf(sys->con, "%03lX  %08X %08X %08X %08X\n",
			   i*sizeof(u32), val[i], val[i+1], val[i+2],
			   val[i+3]);

	return 0;
}

static int cmd_display_gpr(struct virt_sys *sys, char *cmd, int len)
{
	con_printf(sys->con, "GR  0 = %016llx %016llx\n",
		   sys->task->cpu->regs.gpr[0],
		   sys->task->cpu->regs.gpr[1]);
	con_printf(sys->con, "GR  2 = %016llx %016llx\n",
		   sys->task->cpu->regs.gpr[2],
		   sys->task->cpu->regs.gpr[3]);
	con_printf(sys->con, "GR  4 = %016llx %016llx\n",
		   sys->task->cpu->regs.gpr[4],
		   sys->task->cpu->regs.gpr[5]);
	con_printf(sys->con, "GR  6 = %016llx %016llx\n",
		   sys->task->cpu->regs.gpr[6],
		   sys->task->cpu->regs.gpr[7]);
	con_printf(sys->con, "GR  8 = %016llx %016llx\n",
		   sys->task->cpu->regs.gpr[8],
		   sys->task->cpu->regs.gpr[9]);
	con_printf(sys->con, "GR 10 = %016llx %016llx\n",
		   sys->task->cpu->regs.gpr[10],
		   sys->task->cpu->regs.gpr[11]);
	con_printf(sys->con, "GR 12 = %016llx %016llx\n",
		   sys->task->cpu->regs.gpr[12],
		   sys->task->cpu->regs.gpr[13]);
	con_printf(sys->con, "GR 14 = %016llx %016llx\n",
		   sys->task->cpu->regs.gpr[14],
		   sys->task->cpu->regs.gpr[15]);
	return 0;
}

static int cmd_display_fpcr(struct virt_sys *sys, char *cmd, int len)
{
	con_printf(sys->con, "FPCR  = %08x\n", sys->task->cpu->regs.fpcr);
	return 0;
}

static int cmd_display_fpr(struct virt_sys *sys, char *cmd, int len)
{
	con_printf(sys->con, "FR  0 = %016llx %016llx\n",
		   sys->task->cpu->regs.fpr[0],
		   sys->task->cpu->regs.fpr[1]);
	con_printf(sys->con, "FR  2 = %016llx %016llx\n",
		   sys->task->cpu->regs.fpr[2],
		   sys->task->cpu->regs.fpr[3]);
	con_printf(sys->con, "FR  4 = %016llx %016llx\n",
		   sys->task->cpu->regs.fpr[4],
		   sys->task->cpu->regs.fpr[5]);
	con_printf(sys->con, "FR  6 = %016llx %016llx\n",
		   sys->task->cpu->regs.fpr[6],
		   sys->task->cpu->regs.fpr[7]);
	con_printf(sys->con, "FR  8 = %016llx %016llx\n",
		   sys->task->cpu->regs.fpr[8],
		   sys->task->cpu->regs.fpr[9]);
	con_printf(sys->con, "FR 10 = %016llx %016llx\n",
		   sys->task->cpu->regs.fpr[10],
		   sys->task->cpu->regs.fpr[11]);
	con_printf(sys->con, "FR 12 = %016llx %016llx\n",
		   sys->task->cpu->regs.fpr[12],
		   sys->task->cpu->regs.fpr[13]);
	con_printf(sys->con, "FR 14 = %016llx %016llx\n",
		   sys->task->cpu->regs.fpr[14],
		   sys->task->cpu->regs.fpr[15]);
	return 0;
}

static int cmd_display_cr(struct virt_sys *sys, char *cmd, int len)
{
	con_printf(sys->con, "CR  0 = %016llx %016llx\n",
		   sys->task->cpu->sie_cb.gcr[0],
		   sys->task->cpu->sie_cb.gcr[1]);
	con_printf(sys->con, "CR  2 = %016llx %016llx\n",
		   sys->task->cpu->sie_cb.gcr[2],
		   sys->task->cpu->sie_cb.gcr[3]);
	con_printf(sys->con, "CR  4 = %016llx %016llx\n",
		   sys->task->cpu->sie_cb.gcr[4],
		   sys->task->cpu->sie_cb.gcr[5]);
	con_printf(sys->con, "CR  6 = %016llx %016llx\n",
		   sys->task->cpu->sie_cb.gcr[6],
		   sys->task->cpu->sie_cb.gcr[7]);
	con_printf(sys->con, "CR  8 = %016llx %016llx\n",
		   sys->task->cpu->sie_cb.gcr[8],
		   sys->task->cpu->sie_cb.gcr[9]);
	con_printf(sys->con, "CR 10 = %016llx %016llx\n",
		   sys->task->cpu->sie_cb.gcr[10],
		   sys->task->cpu->sie_cb.gcr[11]);
	con_printf(sys->con, "CR 12 = %016llx %016llx\n",
		   sys->task->cpu->sie_cb.gcr[12],
		   sys->task->cpu->sie_cb.gcr[13]);
	con_printf(sys->con, "CR 14 = %016llx %016llx\n",
		   sys->task->cpu->sie_cb.gcr[14],
		   sys->task->cpu->sie_cb.gcr[15]);
	return 0;
}

static int cmd_display_ar(struct virt_sys *sys, char *cmd, int len)
{
	con_printf(sys->con, "AR  0 = %08x %08x\n",
		   sys->task->cpu->regs.ar[0],
		   sys->task->cpu->regs.ar[1]);
	con_printf(sys->con, "AR  2 = %08x %08x\n",
		   sys->task->cpu->regs.ar[2],
		   sys->task->cpu->regs.ar[3]);
	con_printf(sys->con, "AR  4 = %08x %08x\n",
		   sys->task->cpu->regs.ar[4],
		   sys->task->cpu->regs.ar[5]);
	con_printf(sys->con, "AR  6 = %08x %08x\n",
		   sys->task->cpu->regs.ar[6],
		   sys->task->cpu->regs.ar[7]);
	con_printf(sys->con, "AR  8 = %08x %08x\n",
		   sys->task->cpu->regs.ar[8],
		   sys->task->cpu->regs.ar[9]);
	con_printf(sys->con, "AR 10 = %08x %08x\n",
		   sys->task->cpu->regs.ar[10],
		   sys->task->cpu->regs.ar[11]);
	con_printf(sys->con, "AR 12 = %08x %08x\n",
		   sys->task->cpu->regs.ar[12],
		   sys->task->cpu->regs.ar[13]);
	con_printf(sys->con, "AR 14 = %08x %08x\n",
		   sys->task->cpu->regs.ar[14],
		   sys->task->cpu->regs.ar[15]);
	return 0;
}

static int cmd_display_psw(struct virt_sys *sys, char *cmd, int len)
{
	u32 *ptr = (u32*) &sys->task->cpu->sie_cb.gpsw;

	con_printf(sys->con, "PSW = %08x %08x %08x %08x\n",
		   ptr[0], ptr[1], ptr[2], ptr[3]);
	return 0;
}

static struct cpcmd cmd_tbl_display[] = {
	{"STORAGE", cmd_display_storage, NULL},
	{"GPR", cmd_display_gpr, NULL},
	{"FPCR", cmd_display_fpcr, NULL},
	{"FPR", cmd_display_fpr, NULL},
	{"CR", cmd_display_cr, NULL},
	{"AR", cmd_display_ar, NULL},
	{"PSW", cmd_display_psw, NULL},
	{"SIECB", cmd_display_siecb, NULL},
	{NULL, NULL, NULL},
};
