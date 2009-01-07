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

static void display_rdev(struct console *con, struct device *dev)
{
	con_printf(con, "%-4s %04X %04X SCH = %05X\n",
		   type2name(dev->type), dev->ccuu, dev->type,
		   dev->sch);
}

static void display_vdev(struct console *con, struct vdev *vdev)
{
	switch (vdev->type) {
		case VDEV_CONS:
			con_printf(con, "CONS %04X 3215 ON %s %04X SCH = %05X\n",
				   vdev->vdev, type2name(con->dev->type),
				   con->dev->ccuu, vdev->vsch);
			break;
		case VDEV_DED:
			con_printf(con, "???? %04X SCH = %05X\n",
				   vdev->vdev, vdev->vsch);
			break;
		case VDEV_SPOOL:
			con_printf(con, "%-4s %04X %04X SCH = %05X\n",
				   type2name(vdev->u.spool.type),
				   vdev->vdev, vdev->u.spool.type,
				   vdev->vsch);
			break;
		case VDEV_MDISK:
			con_printf(con, "DASD %04X 3390 %6d CYL ON DASD %04X SCH = %05X\n",
				   vdev->vdev,
				   vdev->u.mdisk.cylcnt, /* cyl count */
				   vdev->u.mdisk.rdev,   /* rdev */
				   vdev->vsch);
			break;
		case VDEV_LINK:
			con_printf(con, "LINK %04X TO %s %04X SCH = %05X\n",
				   vdev->vdev,
				   "????", /* userid */
				   0xffff, /* user's vdev # */
				   vdev->vsch);
			break;
		default:
			con_printf(con, "???? unknown device type (%04X, %05X)\n",
				   vdev->vdev, vdev->vsch);
			break;
	}
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
		int i;

		con_printf(sys->con, "CPU %s\n", __guest_state_to_str(sys->task->cpu->state));
		con_printf(sys->con, "STORAGE = %lluM\n", sys->directory->storage_size >> 20);

		for(i=0; sys->directory->devices[i].type != VDEV_INVAL; i++)
			display_vdev(sys->con, &sys->directory->devices[i]);

	} else if (!strcasecmp(cmd, "REAL")) {
		con_printf(sys->con, "CPU RUNNING\n");
		con_printf(sys->con, "STORAGE = %lluM\n", memsize >> 20);

		list_devices(sys->con, display_rdev);

	} else
		con_printf(sys->con, "QUERY: Unknown variable '%s'\n", cmd);

	return 0;
}
