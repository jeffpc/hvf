static char *__guest_state_to_str(enum guest_cpustate st)
{
	switch (st) {
		case GUEST_STOPPED:	return "STOPPED";
		case GUEST_RUNNING:	return "RUNNING";
	}

	return "???";
}

static int cmd_query(struct user *u, char *cmd, int len)
{
	if (!strcasecmp(cmd, "CPLEVEL")) {
		con_printf(u->con, "HVF version " VERSION "\n");
		con_printf(u->con, "IPL at %02d:%02d:%02d UTC %04d-%02d-%02d\n",
			   ipltime.th, ipltime.tm, ipltime.ts, ipltime.dy,
			   ipltime.dm, ipltime.dd);

	} else if (!strcasecmp(cmd, "TIME")) {
		struct datetime dt;

		get_parsed_tod(&dt);

		con_printf(u->con, "TIME IS %02d:%02d:%02d UTC %04d-%02d-%02d\n",
			   dt.th, dt.tm, dt.ts, dt.dy, dt.dm, dt.dd);

	} else if (!strcasecmp(cmd, "VIRTUAL")) {
		int i;

		con_printf(u->con, "CPU %s\n", __guest_state_to_str(current->guest->state));
		con_printf(u->con, "STORAGE = %lluM\n", u->storage_size >> 20);

		for(i=0; u->devices[i].type != VDEV_INVAL; i++) {
			switch (u->devices[i].type) {
				case VDEV_CONS:
					con_printf(u->con, "CONS %04x 3215 ON %s %04x SCH = %05x\n",
						   u->devices[i].vdev,
						   type2name(u->con->dev->type),
						   u->con->dev->ccuu,
						   u->devices[i].vsch);
					break;
				case VDEV_DED:
					con_printf(u->con, "???? %04x SCH = %05x\n",
						   u->devices[i].vdev,
						   u->devices[i].vsch);
					break;
				case VDEV_SPOOL:
					con_printf(u->con, "%-4s %04x %04x SCH = %05x\n",
						   type2name(u->devices[i].u.spool.type),
						   u->devices[i].vdev,
						   u->devices[i].u.spool.type,
						   u->devices[i].vsch);
					break;
				case VDEV_MDISK:
					con_printf(u->con, "DASD %04x 3390 %6d CYL ON DASD %04x SCH = %05x\n",
						   u->devices[i].vdev,
						   u->devices[i].u.mdisk.cylcnt,/* cyl count */
						   u->devices[i].u.mdisk.rdev,	/* rdev */
						   u->devices[i].vsch);
					break;
				case VDEV_LINK:
					con_printf(u->con, "LINK %04x TO %s %04x SCH = %05x\n",
						   u->devices[i].vdev,
						   "????", /* userid */
						   0xffff, /* user's vdev # */
						   u->devices[i].vsch);
					break;
				default:
					con_printf(u->con, "???? unknown device type (%04x, %05x)\n",
						   u->devices[i].vdev,
						   u->devices[i].vsch);
					break;
			}
		}

	} else
		con_printf(u->con, "QUERY: Unknown variable '%s'\n", cmd);

	return 0;
}
