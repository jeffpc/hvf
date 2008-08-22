static int parse_addrspec(u64 *ret, char *s, int len)
{
	u64 tmp;

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
	}

	*ret = tmp;
	return 0;
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
