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
static int cmd_display_storage(struct user *u, char *cmd, int len)
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
			con_printf(u->con, "DISPLAY: Invalid length/type '%s'\n",
				   cmd);
			return -EINVAL;
	}

	ret = parse_addrspec(&guest_addr, cmd, len);
	if (ret) {
		con_printf(u->con, "DISPLAY: Invalid addr-spec '%s'\n", cmd);
		return ret;
	}

	/*
	 * round down to the nearest word/dword/etc - only if not
	 * disassembling
	 */
	if (!dis)
		guest_addr &= ~((u64) mlen-1);

	con_printf(u->con, "guest:  addr = %llx; len = %d\n", guest_addr, mlen);

	/* walk the page tables to find the real page frame */
	ret = virt2phy(&current->guest->as, guest_addr, &host_addr);
	if (ret) {
		con_printf(u->con, "DISPLAY: Specified address is not part of "
			   "guest configuration\n");
		return ret;
	}

	con_printf(u->con, "host:   addr = %llx\n", host_addr);

	if (dis) {
		char buf[64];

		/*
		 * FIXME: make sure crossing page-boundary doesn't break
		 * give us garbage
		 */
		mlen = disassm((unsigned char*) host_addr, buf, 64);

		if ((mlen > 2) && ((host_addr & (PAGE_SIZE-1)) >= (PAGE_SIZE-2))) {
			con_printf(u->con, "DISPLAY: Instruction spans page "
				   "boundary - not supported\n");
			return 0;
		}

		/* load the dword at the address, and shift it to the LSB part */
		val = *((u64*)host_addr) >> 8*(sizeof(u64) - mlen);

		con_printf(u->con, "R%016llX  %0*llX%*s  %s\n", guest_addr,
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
			con_printf(u->con, "R%016llX  %0*llX\n", guest_addr,
				   mlen << 1, val);
		else if (mlen == 8)
			con_printf(u->con, "R%016llX  %08llX %08llX\n", guest_addr,
				   val >> 32, val & 0xffffffffULL);
		else
			con_printf(u->con, "DISPLAY: unhandled display length\n");
	}

	return 0;
}

static int cmd_display_gpr(struct user *u, char *cmd, int len)
{
	con_printf(u->con, "GR  0 = %016llx %016llx\n",
		   current->guest->regs.gpr[0],
		   current->guest->regs.gpr[1]);
	con_printf(u->con, "GR  2 = %016llx %016llx\n",
		   current->guest->regs.gpr[2],
		   current->guest->regs.gpr[3]);
	con_printf(u->con, "GR  4 = %016llx %016llx\n",
		   current->guest->regs.gpr[4],
		   current->guest->regs.gpr[5]);
	con_printf(u->con, "GR  6 = %016llx %016llx\n",
		   current->guest->regs.gpr[6],
		   current->guest->regs.gpr[7]);
	con_printf(u->con, "GR  8 = %016llx %016llx\n",
		   current->guest->regs.gpr[8],
		   current->guest->regs.gpr[9]);
	con_printf(u->con, "GR 10 = %016llx %016llx\n",
		   current->guest->regs.gpr[10],
		   current->guest->regs.gpr[11]);
	con_printf(u->con, "GR 12 = %016llx %016llx\n",
		   current->guest->regs.gpr[12],
		   current->guest->regs.gpr[13]);
	con_printf(u->con, "GR 14 = %016llx %016llx\n",
		   current->guest->regs.gpr[14],
		   current->guest->regs.gpr[15]);
	return 0;
}

static int cmd_display_psw(struct user *u, char *cmd, int len)
{
	u32 *ptr = (u32*) &current->guest->sie_cb.gpsw;

	con_printf(u->con, "PSW = %08x %08x %08x %08x\n",
		   ptr[0], ptr[1], ptr[2], ptr[3]);
	return 0;
}

static struct cpcmd cmd_tbl_display[] = {
	{"STORAGE", cmd_display_storage, NULL},
	{"GPR", cmd_display_gpr, NULL},
#if 0
	{"FPR", cmd_display_fpr, NULL},
	{"CR", cmd_display_cr, NULL},
	{"AR", cmd_display_ar, NULL},
#endif
	{"PSW", cmd_display_psw, NULL},
	{NULL, NULL, NULL},
};
