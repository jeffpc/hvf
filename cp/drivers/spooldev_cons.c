#include <vcpu.h>
#include <vdevice.h>

static int open_spoolfile(struct virt_sys *sys, struct virt_device *vdev)
{
	struct spool_file *f;

	if (vdev->u.spool.file)
		return 0;

	f = alloc_spool();
	if (IS_ERR(f))
		return PTR_ERR(f);

	vdev->u.spool.file = f;
	return 0;
}

static void cons_write(struct virt_sys *sys, struct virt_device *vdev,
		       struct ccw *ccw, struct spdev_state *st)
{
	u64 len;
	int ret;

	/* all the exit paths signal Channel and Device End */
	st->dev_status = SC_STATUS_CE | SC_STATUS_DE;

	if (ccw->cmd != 0x01)
		goto cmdrej;

	ret = open_spoolfile(sys, vdev);
	if (ret) {
		con_printf(sys->con, "Could not allocate a new spool file "
			   "for device %04x (%s)\n", vdev->pmcw.dev_num,
			   errstrings[ret]);
		goto cmdrej;
	}

	len = min_t(u64, ccw->count, 150);

	ret = spool_append_rec(vdev->u.spool.file, st->iobuf, len);
	if (ret) {
		con_printf(sys->con, "Failed to append rec to spool file "
			   "for device %04x (%s)\n", vdev->pmcw.dev_num,
			   errstrings[ret]);
		goto cmdrej;
	}

	st->rem -= len;
	st->pos += len;

	return;

cmdrej:
	st->dev_status |= SC_STATUS_UC;
	vdev->sense = SENSE_CMDREJ;
}

struct spool_ops spool_cons_ops = {
	.f		= {
		[0x1]		= cons_write,
	},
};

