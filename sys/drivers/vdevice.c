#include <slab.h>
#include <list.h>
#include <vdevice.h>

int alloc_virt_dev(struct virt_sys *sys, struct directory_vdev *dirdev,
		   u32 sch)
{
	struct virt_device *vdev;

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
			goto free;
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

	return 0;

free:
	free(vdev);
	return 0;

out:
	return -EINVAL;
}
