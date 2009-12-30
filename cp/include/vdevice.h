#ifndef __VDEVICE_H
#define __VDEVICE_H

#include <list.h>
#include <directory.h>
#include <sched.h>

struct virt_device {
	struct list_head devices;
	enum directory_vdevtype vtype;	/* VDEV_CONS, VDEV_DED, ... */
	u32 sch;			/* subchannel id */
	u16 type;			/* 3330, 3215, ... */
	u8 model;

	union {
		struct {
			struct device *rdev;
					/* real device */
		} dedicate;
	} u;

	struct pmcw pmcw;		/* path info */
	struct scsw scsw;		/* subchannel-status */
};

extern int alloc_virt_dev(struct virt_sys *sys,
		struct directory_vdev *dirdev, u32 sch);

#endif
