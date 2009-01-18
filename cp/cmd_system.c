extern u32 GUEST_IPL_CODE[];
extern u32 GUEST_IPL_REGSAVE[];

/*
 *!!! IPL
 *!p >>--IPL-----------------------------------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Perform a ...
 *!! NOTES
 *! Not yet implemented.
 */
static int cmd_ipl(struct virt_sys *sys, char *cmd, int len)
{
	u64 host_addr;
	int bytes;
	int ret;
	int i;

	bytes = sizeof(u32)*(GUEST_IPL_REGSAVE-GUEST_IPL_CODE);

	con_printf(sys->con, "WARNING: IPL command is work-in-progress\n");

	/*
	 * FIXME: this should be conditional based whether or not we were
	 * told to clear
	 */
	guest_load_normal(sys);

	sys->task->cpu->state = GUEST_LOAD;

	ret = virt2phy_current(GUEST_IPL_BASE, &host_addr);
	if (ret)
		goto fail;

	/* we can't go over a page! */
	BUG_ON((bytes+(16*32)) > PAGE_SIZE);

	/* copy the helper into the guest storage */
	memcpy((void*) host_addr, GUEST_IPL_CODE, bytes);

	/* save the current guest regs */
	for (i=0; i<16; i++) {
		u32 *ptr = (u32*) ((u8*) host_addr + bytes);

		ptr[i] = (u32) sys->task->cpu->regs.gpr[i];
	}

	sys->task->cpu->regs.gpr[1]  = GUEST_IPL_SCHNUM;
	sys->task->cpu->regs.gpr[2]  = GUEST_IPL_DEVNUM;
	sys->task->cpu->regs.gpr[12] = GUEST_IPL_BASE;

	*((u64*) &sys->task->cpu->sie_cb.gpsw) = 0x0008000080000000ULL |
						 GUEST_IPL_BASE;

	sys->task->cpu->state = GUEST_STOPPED;

	con_printf(sys->con, "GUEST IPL HELPER LOADED; ENTERED STOPPED STATE\n");

	return 0;

fail:
	sys->task->cpu->state = GUEST_STOPPED;
	return ret;
}

/*!!! SYSTEM CLEAR
 *!p >>--SYSTEM--CLEAR-------------------------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Identical to reset-clear button on a real mainframe.
 *
 *!!! SYSTEM RESET
 *!p >>--SYSTEM--RESET-------------------------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Identical to reset-normal button on a real mainframe.
 *
 *!!! SYSTEM RESTART
 *!p >>--SYSTEM--RESTART-----------------------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Perform a restart operation.
 *!! NOTES
 *! Not yet implemented.
 *
 *!!! SYSTEM STORE
 *!p >>--SYSTEM--STORE-------------------------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Perform a ...
 *!! NOTES
 *! Not yet implemented.
 */
static int cmd_system(struct virt_sys *sys, char *cmd, int len)
{
	if (!strcasecmp(cmd, "CLEAR")) {
		guest_system_reset_clear(sys);
		con_printf(sys->con, "STORAGE CLEARED - SYSTEM RESET\n");
	} else if (!strcasecmp(cmd, "RESET")) {
		guest_system_reset_normal(sys);
		con_printf(sys->con, "SYSTEM RESET\n");
	} else if (!strcasecmp(cmd, "RESTART")) {
		con_printf(sys->con, "SYSTEM RESTART is not yet supported\n");
	} else if (!strcasecmp(cmd, "STORE")) {
		con_printf(sys->con, "SYSTEM STORE is not yet supported\n");
	} else
		con_printf(sys->con, "SYSTEM: Unknown variable '%s'\n", cmd);

	return 0;
}
