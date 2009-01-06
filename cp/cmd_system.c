extern u32 GUEST_IPL_CODE[];
extern u32 GUEST_IPL_REGSAVE[];

static int cmd_ipl(struct user *u, char *cmd, int len)
{
	u64 host_addr;
	int bytes;
	int ret;
	int i;

	bytes = sizeof(u32)*(GUEST_IPL_REGSAVE-GUEST_IPL_CODE);

	con_printf(u->con, "WARNING: IPL command is work-in-progress\n");

	/*
	 * FIXME: this should be conditional based whether or not we were
	 * told to clear
	 */
	guest_load_normal(u);

	current->guest->state = GUEST_LOAD;

	ret = virt2phy(&current->guest->as, GUEST_IPL_BASE, &host_addr);
	if (ret)
		goto fail;

	/* we can't go over a page! */
	BUG_ON((bytes+(16*32)) > PAGE_SIZE);

	/* copy the helper into the guest storage */
	memcpy((void*) host_addr, GUEST_IPL_CODE, bytes);

	/* save the current guest regs */
	for (i=0; i<16; i++) {
		u32 *ptr = (u32*) ((u8*) host_addr + bytes);

		ptr[i] = (u32) current->guest->regs.gpr[i];
	}

	current->guest->regs.gpr[1]  = GUEST_IPL_SCHNUM;
	current->guest->regs.gpr[2]  = GUEST_IPL_DEVNUM;
	current->guest->regs.gpr[12] = GUEST_IPL_BASE;

	*((u64*) &current->guest->sie_cb.gpsw) = 0x0008000080000000 |
						 GUEST_IPL_BASE;

	current->guest->state = GUEST_STOPPED;

	con_printf(u->con, "GUEST IPL HELPER LOADED; ENTERED STOPPED STATE\n");

	return 0;

fail:
	current->guest->state = GUEST_STOPPED;
	return ret;
}

static int cmd_system(struct user *u, char *cmd, int len)
{
	if (!strcasecmp(cmd, "CLEAR")) {
		guest_system_reset_clear(u);
		con_printf(u->con, "STORAGE CLEARED - SYSTEM RESET\n");
	} else if (!strcasecmp(cmd, "RESET")) {
		guest_system_reset_normal(u);
		con_printf(u->con, "SYSTEM RESET\n");
	} else if (!strcasecmp(cmd, "RESTART")) {
		con_printf(u->con, "SYSTEM RESTART is not yet supported\n");
	} else if (!strcasecmp(cmd, "STORE")) {
		con_printf(u->con, "SYSTEM STORE is not yet supported\n");
	} else
		con_printf(u->con, "SYSTEM: Unknown variable '%s'\n", cmd);

	return 0;
}
