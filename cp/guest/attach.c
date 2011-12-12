/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <vcpu.h>
#include <guest.h>
#include <sclp.h>

static void guest_append_crw(struct virt_sys *sys, struct crw *crw)
{
	sclp_msg("FIXME: guest CRW was not queued");
}

int guest_attach(struct virt_sys *sys, u64 rdev, u64 vdev)
{
	struct directory_vdev dv = {
		.type = VDEV_DED,
		.vdev = vdev,
		.u.dedicate.rdev = rdev,
	};
	struct virt_device *cur;
	struct crw crw;
	int found;
	int ret;
	u64 sch;

	mutex_lock(&sys->virt_devs_lock);

	/* are we supposed to pick a vdev number? */
	if (vdev == -1) {
		/* TODO: this is a super-stupid algorithm - fix it */
		for(vdev=0x0000; vdev<=0xffff; vdev++) {
			found = 0;
			list_for_each_entry(cur, &sys->virt_devs, devices) {
				if (cur->pmcw.dev_num == vdev) {
					found = 1;
					break;
				}
			}

			if (!found)
				break;
		}

		if (found)
			goto out_err;

		dv.vdev = vdev;
	}

	/* pick a subchannel number
	 * TODO: this is a super-stupid algorithm - fix it
	 */
	for(sch=0x10000; sch<=0x1ffff; sch++) {
		found = 0;
		list_for_each_entry(cur, &sys->virt_devs, devices) {
			if (cur->sch == sch) {
				found = 1;
				break;
			}
		}

		if (!found)
			break;
	}

	if (found)
		goto out_err;

	memset(&crw, 0, sizeof(struct crw));
	crw.rsc = 0x3;
	crw.erc = 0x4;
	crw.id  = sch & ~0x10000;

	/* add the device */
	ret = alloc_virt_dev(sys, &dv, sch);

	if (!ret)
		guest_append_crw(sys, &crw);

	mutex_unlock(&sys->virt_devs_lock);
	return ret;

out_err:
	mutex_unlock(&sys->virt_devs_lock);
	return -EBUSY;
}
