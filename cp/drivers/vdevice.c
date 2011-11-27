/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <slab.h>
#include <list.h>
#include <vcpu.h>
#include <vdevice.h>

static int __setup_vdev_ded(struct virt_sys *sys,
			    struct directory_vdev *dirdev,
			    struct virt_device *vdev)
{
	struct device *rdev;

	rdev = find_device_by_ccuu(dirdev->u.dedicate.rdev);
	if (IS_ERR(rdev))
		return PTR_ERR(rdev);

	mutex_lock(&rdev->lock);
	rdev->in_use = 1;
	mutex_unlock(&rdev->lock);

	vdev->u.dedicate.rdev = rdev;
	vdev->type = rdev->type;
	vdev->model = rdev->model;

	return 0;
}

static LOCK_CLASS(vdev_lc);

int alloc_virt_dev(struct virt_sys *sys, struct directory_vdev *dirdev,
		   u32 sch)
{
	struct virt_device *vdev;
	int ret = 0;

	vdev = malloc(sizeof(struct virt_device), ZONE_NORMAL);
	if (!vdev)
		return -ENOMEM;

	mutex_init(&vdev->lock, &vdev_lc);

	vdev->vtype = dirdev->type;
	vdev->sch = sch;
	vdev->pmcw.v = 1;
	vdev->pmcw.dev_num = dirdev->vdev;
	vdev->pmcw.lpm = 0x80;
	vdev->pmcw.pim = 0x80;
	vdev->pmcw.pom = 0xff;
	vdev->pmcw.pam = 0x80;

	switch(dirdev->type) {
		case VDEV_CONS:
			/* CONS is really just a special case of SPOOL */
			vdev->u.spool.file = NULL;
			vdev->u.spool.ops = &spool_cons_ops;
			vdev->type = 0x3215;
			vdev->model = 0;
			break;
		case VDEV_SPOOL:
			vdev->u.spool.file = NULL;
			vdev->u.spool.ops = NULL;
			vdev->type = dirdev->u.spool.type;
			vdev->model = dirdev->u.spool.model;
			break;
		case VDEV_DED:
			ret = __setup_vdev_ded(sys, dirdev, vdev);
			break;
		case VDEV_MDISK:
			vdev->type = 0x3390;
			vdev->model = 3;
			// FIXME: hook it up to mdisk driver
			break;
		case VDEV_LINK:
		case VDEV_INVAL:
			ret = -EINVAL;
			break;
	}

	if (!ret)
		list_add_tail(&vdev->devices, &sys->virt_devs);
	else
		free(vdev);

	return ret;
}
