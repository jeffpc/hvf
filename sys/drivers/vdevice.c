#include <slab.h>
#include <list.h>
#include <vdevice.h>

static int __setup_vdev_ded(struct virt_sys *sys,
			    struct directory_vdev *dirdev,
			    struct virt_device *vdev)
{
	struct device *rdev;

	rdev = find_device_by_ccuu(dirdev->u.dedicate.rdev);
	if (IS_ERR(rdev))
		return PTR_ERR(rdev);

	atomic_inc(&rdev->in_use);

	vdev->u.dedicate.rdev = rdev;
	vdev->type = rdev->type;
	vdev->model = rdev->model;

	return 0;
}

int alloc_virt_dev(struct virt_sys *sys, struct directory_vdev *dirdev,
		   u32 sch)
{
	struct virt_device *vdev;
	int ret = 0;

	vdev = malloc(sizeof(struct virt_device), ZONE_NORMAL);
	if (!vdev)
		return -ENOMEM;

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
			vdev->type = 0x3215;
			vdev->model = 0;
			break;
		case VDEV_DED:
			ret = __setup_vdev_ded(sys, dirdev, vdev);
			break;
		case VDEV_SPOOL:
			vdev->type = dirdev->u.spool.type;
			vdev->model = dirdev->u.spool.model;
			// FIXME: hook it up to the spooler
			break;
		case VDEV_MDISK:
			vdev->type = 0x3390;
			vdev->model = 3;
			// FIXME: hook it up to mdisk driver
			break;
		case VDEV_LINK:
			goto free;
		case VDEV_INVAL:
			goto out;
	}

	list_add_tail(&vdev->devices, &sys->virt_devs);

	return ret;

free:
	free(vdev);
	return 0;

out:
	free(vdev);
	return -EINVAL;
}
