/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __VDEVICE_H
#define __VDEVICE_H

#include <list.h>
#include <directory.h>
#include <sched.h>
#include <mutex.h>

struct virt_device {
	struct list_head devices;

	/*
	 * protects the R/W fields:
	 *  - pmcw
	 *  - scsw
	 */
	mutex_t lock;

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
