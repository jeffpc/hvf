static char *__guest_state_to_str(enum virt_cpustate st)
{
	switch (st) {
		case GUEST_STOPPED:	return "STOPPED";
		case GUEST_OPERATING:	return "RUNNING";
		case GUEST_LOAD:	return "LOADING";
		case GUEST_CHECKSTOP:	return "CHECK-STOP";
	}

	return "???";
}

static int cmd_query(struct virt_sys *sys, char *cmd, int len)
{
	if (!strcasecmp(cmd, "CPLEVEL")) {
		con_printf(sys->con, "HVF version " VERSION "\n");
		con_printf(sys->con, "IPL at %02d:%02d:%02d UTC %04d-%02d-%02d\n",
			   ipltime.th, ipltime.tm, ipltime.ts, ipltime.dy,
			   ipltime.dm, ipltime.dd);

	} else if (!strcasecmp(cmd, "TIME")) {
		struct datetime dt;

		get_parsed_tod(&dt);

		con_printf(sys->con, "TIME IS %02d:%02d:%02d UTC %04d-%02d-%02d\n",
			   dt.th, dt.tm, dt.ts, dt.dy, dt.dm, dt.dd);

	} else if (!strcasecmp(cmd, "VIRTUAL")) {
		struct vdev *vdev;
		int i;

		con_printf(sys->con, "CPU %s\n", __guest_state_to_str(sys->task->cpu->state));
		con_printf(sys->con, "STORAGE = %lluM\n", sys->directory->storage_size >> 20);

		for(i=0; sys->directory->devices[i].type != VDEV_INVAL; i++) {
			vdev = &sys->directory->devices[i];

			switch (vdev->type) {
				case VDEV_CONS:
					con_printf(sys->con, "CONS %04x 3215 ON %s %04x SCH = %05x\n",
						   vdev->vdev, type2name(sys->con->dev->type),
						   sys->con->dev->ccuu, vdev->vsch);
					break;
				case VDEV_DED:
					con_printf(sys->con, "???? %04x SCH = %05x\n",
						   vdev->vdev, vdev->vsch);
					break;
				case VDEV_SPOOL:
					con_printf(sys->con, "%-4s %04x %04x SCH = %05x\n",
						   type2name(vdev->u.spool.type),
						   vdev->vdev, vdev->u.spool.type,
						   vdev->vsch);
					break;
				case VDEV_MDISK:
					con_printf(sys->con, "DASD %04x 3390 %6d CYL ON DASD %04x SCH = %05x\n",
						   vdev->vdev,
						   vdev->u.mdisk.cylcnt,/* cyl count */
						   vdev->u.mdisk.rdev,	/* rdev */
						   vdev->vsch);
					break;
				case VDEV_LINK:
					con_printf(sys->con, "LINK %04x TO %s %04x SCH = %05x\n",
						   vdev->vdev,
						   "????", /* userid */
						   0xffff, /* user's vdev # */
						   vdev->vsch);
					break;
				default:
					con_printf(sys->con, "???? unknown device type (%04x, %05x)\n",
						   vdev->vdev, vdev->vsch);
					break;
			}
		}

	} else
		con_printf(sys->con, "QUERY: Unknown variable '%s'\n", cmd);

	return 0;
}
