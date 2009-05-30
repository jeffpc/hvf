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

static void display_vdev(struct console *con, struct virt_device *vdev)
{
	switch (vdev->vtype) {
		case VDEV_CONS:
			con_printf(con, "CONS %04X 3215 ON %s %04X SCH = %05X\n",
				   vdev->pmcw.dev_num,
				   type2name(con->dev->type), // FIXME?
				   con->dev->ccuu, vdev->sch);
			break;
		case VDEV_DED:
			con_printf(con, "???? %04X SCH = %05X\n",
				   vdev->pmcw.dev_num, vdev->sch);
			break;
		case VDEV_SPOOL:
			con_printf(con, "%-4s %04X %04X SCH = %05X\n",
				   type2name(vdev->type),
				   vdev->pmcw.dev_num, vdev->type,
				   vdev->sch);
			break;
		case VDEV_MDISK:
			con_printf(con, "DASD %04X 3390 %6d CYL ON DASD %04X SCH = %05X\n",
				   vdev->pmcw.dev_num,
				   0, /* FIXME: cyl count */
				   0, /* FIXME: rdev */
				   vdev->sch);
			break;
		case VDEV_LINK:
			con_printf(con, "LINK %04X TO %s %04X SCH = %05X\n",
				   vdev->pmcw.dev_num,
				   "????", /* FIXME: userid */
				   0xffff, /* FIXME: user's vdev # */
				   vdev->sch);
			break;
		default:
			con_printf(con, "???? unknown device type (%04X, %05X)\n",
				   vdev->pmcw.dev_num, vdev->sch);
			break;
	}
}

static void display_task(struct console *con, struct task *task)
{
	con_printf(con, "%*s %p %c\n", -TASK_NAME_LEN, task->name,
		   task, (task->state == TASK_RUNNING ? 'R' : 'S'));
}

/*
 *!!! QUERY CPLEVEL
 *!p >>--QUERY--CPLEVEL------------------------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Displays the HVF version and time of IPL
 *
 *!!! QUERY TIME
 *!p >>--QUERY--TIME---------------------------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Displays the current time
 *
 *!!! QUERY VIRTUAL
 *!p >>--QUERY--VIRTUAL------------------------------------------------------------><
 *!! AUTH G
 *!! PURPOSE
 *! Lists all of the guest's virtual devices
 *
 *!!! QUERY REAL
 *!p >>--QUERY--REAL---------------------------------------------------------------><
 *!! AUTH A
 *!! PURPOSE
 *! Lists all of the host's real devices
 *
 *!!! QUERY TASK
 *!p >>--QUERY--TASK---------------------------------------------------------------><
 *!! AUTH A
 *!! PURPOSE
 *! Lists all of the tasks running on the host. This includes guest virtual
 *! cpu tasks, as well as system helper tasks.
 */
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
		struct virt_device *vdev;

		con_printf(sys->con, "CPU 00  ID  %016llX %s\n",
			   sys->task->cpu->cpuid,
			   __guest_state_to_str(sys->task->cpu->state));
		con_printf(sys->con, "STORAGE = %lluM\n", sys->directory->storage_size >> 20);

		list_for_each_entry(vdev, &sys->virt_devs, devices)
			display_vdev(sys->con, vdev);

	} else if (!strcasecmp(cmd, "REAL")) {
		con_printf(sys->con, "CPU %02d  ID  %016llX RUNNING\n",
			   getcpuaddr(),
			   getcpuid());
		con_printf(sys->con, "STORAGE = %lluM\n", memsize >> 20);

		list_devices(sys->con, display_rdev);

	} else if (!strcasecmp(cmd, "TASK")) {
		list_tasks(sys->con, display_task);

	} else
		con_printf(sys->con, "QUERY: Unknown variable '%s'\n", cmd);

	return 0;
}
