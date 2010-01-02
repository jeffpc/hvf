/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

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
	char buf[40];

	buf[0] = '\0';

	if (dev->dev && dev->dev->snprintf)
		dev->dev->snprintf(dev, buf, 40);

	con_printf(con, "%-4s %04X %04X %s%sSCH = %05X\n",
		   type2name(dev->type), dev->ccuu, dev->type, buf,
		   atomic_read(&dev->in_use) ? "" : "FREE ",
		   dev->sch);
}

static void display_vdev(struct console *con, struct virt_device *vdev)
{
	switch (vdev->vtype) {
		case VDEV_CONS:
			con_printf(con, "CONS %04X 3215 ON %s %04X %s SCH = %05X\n",
				   vdev->pmcw.dev_num,
				   type2name(con->dev->type), // FIXME?
				   con->dev->ccuu,
				   con->sys->print_ts ? "TS" : "NOTS",
				   vdev->sch);
			break;
		case VDEV_DED:
			con_printf(con, "%-4s %04X %04X ON DEV %04X SCH = %05X\n",
				   type2name(vdev->type),
				   vdev->pmcw.dev_num,
				   vdev->type,
				   vdev->u.dedicate.rdev->ccuu,
				   vdev->sch);
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
	char state;

	switch(task->state) {
		case TASK_RUNNING:  state = 'R'; break;
		case TASK_SLEEPING: state = 'S'; break;
		case TASK_LOCKED:   state = 'L'; break;
		default:            state = '?'; break;
	}

	con_printf(con, "%*s %p %c %016llX\n", -TASK_NAME_LEN, task->name,
		   task, state, task->regs.psw.ptr);
}

/*
 *!!! QUERY TIME
 *!! SYNTAX
 *! \tok{\sc Query} \tok{\sc TIME}
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Displays the current time
 */
static int cmd_query_cplevel(struct virt_sys *sys, char *cmd, int len)
{
	con_printf(sys->con, "HVF version " VERSION "\n");
	con_printf(sys->con, "IPL at %02d:%02d:%02d UTC %04d-%02d-%02d\n",
		   ipltime.th, ipltime.tm, ipltime.ts, ipltime.dy,
		   ipltime.dm, ipltime.dd);

	return 0;
}

/*
 *!!! QUERY CPLEVEL
 *!! SYNTAX
 *! \tok{\sc Query} \tok{\sc CPLEVEL}
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Displays the HVF version and time of IPL
 */
static int cmd_query_time(struct virt_sys *sys, char *cmd, int len)
{
	struct datetime dt;

	get_parsed_tod(&dt);

	con_printf(sys->con, "TIME IS %02d:%02d:%02d UTC %04d-%02d-%02d\n",
		   dt.th, dt.tm, dt.ts, dt.dy, dt.dm, dt.dd);

	return 0;
}

/*
 *!!! QUERY ARCHMODE
 *!! SYNTAX
 *! \tok{\sc Query} \tok{\sc ARCHMODE}
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Displays the virtual machine's current architecture mode.
 */
static int cmd_query_archmode(struct virt_sys *sys, char *cmd, int len)
{
	char *mode = (VCPU_ZARCH(sys->task->cpu)) ? "z/Arch" : "ESA390";

	con_printf(sys->con, "ARCHMODE = %s\n", mode);

	return 0;
}

/*
 *!!! QUERY VIRTUAL
 *!! SYNTAX
 *! \tok{\sc Query} \tok{\sc Virtual}
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Lists all of the guest's virtual devices
 */
static int cmd_query_virtual(struct virt_sys *sys, char *cmd, int len)
{
	struct virt_device *vdev;

	con_printf(sys->con, "CPU 00  ID  %016llX %s\n",
		   sys->task->cpu->cpuid,
		   __guest_state_to_str(sys->task->cpu->state));
	con_printf(sys->con, "STORAGE = %lluM\n", sys->directory->storage_size >> 20);

	list_for_each_entry(vdev, &sys->virt_devs, devices)
		display_vdev(sys->con, vdev);

	return 0;
}

/*
 *!!! QUERY REAL
 *!! SYNTAX
 *! \tok{\sc Query} \tok{\sc Real}
 *!! XATNYS
 *!! AUTH A
 *!! PURPOSE
 *! Lists all of the host's real devices
 */
static int cmd_query_real(struct virt_sys *sys, char *cmd, int len)
{
	SHELL_CMD_AUTH(sys, 'A');

	con_printf(sys->con, "CPU %02d  ID  %016llX RUNNING\n",
		   getcpuaddr(),
		   getcpuid());
	con_printf(sys->con, "STORAGE = %lluM\n", memsize >> 20);

	list_devices(sys->con, display_rdev);

	return 0;
}

/*
 *!!! QUERY TASK
 *!! SYNTAX
 *! \tok{\sc Query} \tok{\sc Task}
 *!! XATNYS
 *!! AUTH A
 *!! PURPOSE
 *! Lists all of the tasks running on the host. This includes guest virtual
 *! cpu tasks, as well as system helper tasks.
 */
static int cmd_query_task(struct virt_sys *sys, char *cmd, int len)
{
	SHELL_CMD_AUTH(sys, 'A');

	list_tasks(sys->con, display_task);

	return 0;
}

static void display_names(struct console *con, struct virt_sys *sys)
{
	con_printf(con, "%s %04X %-8s\n", type2name(sys->con->dev->type),
		   sys->con->dev->ccuu, sys->directory->userid);
}

/*
 *!!! QUERY NAMES
 *!! SYNTAX
 *! \tok{\sc Query} \tok{\sc NAMes}
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Lists all of the logged in users.
 */
static int cmd_query_names(struct virt_sys *sys, char *cmd, int len)
{
	list_users(sys->con, display_names);

	return 0;
}

static struct cpcmd cmd_tbl_query[] = {
	{"ARCHMODE",	cmd_query_archmode,	NULL},

	{"CPLEVEL",	cmd_query_cplevel,	NULL},

	{"TIME",	cmd_query_time,		NULL},

	{"VIRTUAL",	cmd_query_virtual,	NULL},
	{"VIRTUA",	cmd_query_virtual,	NULL},
	{"VIRTU",	cmd_query_virtual,	NULL},
	{"VIRT",	cmd_query_virtual,	NULL},
	{"VIR",		cmd_query_virtual,	NULL},
	{"VI",		cmd_query_virtual,	NULL},
	{"V",		cmd_query_virtual,	NULL},

	{"REAL",	cmd_query_real,		NULL},
	{"REA",		cmd_query_real,		NULL},
	{"RE",		cmd_query_real,		NULL},
	{"R",		cmd_query_real,		NULL},

	{"NAMES",	cmd_query_names,	NULL},
	{"NAME",	cmd_query_names,	NULL},
	{"NAM",		cmd_query_names,	NULL},

	{"TASK",	cmd_query_task,		NULL},
	{"",		NULL,			NULL},
};
