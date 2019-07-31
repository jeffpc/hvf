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
#include <spool.h>
#include <vcpu.h>

/*
 * VDEV_SPOOL and VDEV_CONS related definitions
 */
struct virt_device;
struct spdev_state;

struct spool_ops {
	void (*f[16])(struct virt_sys *sys, struct virt_device *vdev,
		      struct ccw *ccw, struct spdev_state *st);
};

extern struct spool_ops spool_cons_ops;		/* for VDEV_CONS */

struct spdev_state {
	u8 dev_status;	/* device/CU status */
	u8 sch_status;  /* subchannel status */

	u32 addr;	/* ccw address */
	int f;		/* ccw format */

	u8 *iobuf;

	u32 rem;	/* current record remaining count */
	u32 pos;	/* current record remaining offset */

	int cd;		/* chain data bool */
	u8  cmd;	/* chain data command */

	bool tic2tic;

	struct spool_ops *ops;
};

/*
 * The virtual device struct definition
 */
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
		struct {
			struct spool_file *file;
					/* the spool file backing this dev */
			struct spool_ops *ops;
					/* ops vector used for CCW exec */
		} spool;
	} u;

	u8 sense;			/* sense info */

	struct pmcw pmcw;		/* path info */
	struct scsw scsw;		/* subchannel-status */
	struct orb orb;
};

/*
 * VDEV_SPOOL and VDEV_CONS: generate command reject
 */
static inline void spooldev_cmdrej(struct virt_sys *sys,
				   struct virt_device *vdev,
				   struct spdev_state *st)
{
	st->dev_status = SC_STATUS_CE | SC_STATUS_DE | SC_STATUS_UC;
	vdev->sense = SENSE_CMDREJ;
}

/*
 * misc virtual device functions
 */
extern int alloc_virt_dev(struct virt_sys *sys,
		struct directory_vdev *dirdev, u32 sch);

#define for_each_vdev(sys, v)	list_for_each_entry((v), &(sys)->virt_devs, \
						    devices)

#endif
