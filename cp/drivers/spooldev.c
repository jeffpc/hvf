/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <dat.h>
#include <channel.h>
#include <vdevice.h>
#include <spool.h>
#include <buddy.h>

/*
 * store == 0: iobuf -> storage
 * store == 1: storage -> iobuf
 */
static int __xfer_storage(struct virt_sys *sys, struct virt_device *vdev,
			  struct spdev_state *st, struct ccw *ccw, int write)
{
	u64 gaddr, len;
	int ret;

	BUG_ON(ccw->flags & (CCW_FLAG_IDA | CCW_FLAG_MIDA));

	gaddr = ccw->addr; /* the address has already been checked */
	len   = ccw->count;

	if (!len)
		return 0;

	if (gaddr > sys->directory->storage_size) {
		st->sch_status = SC_PROG_CHK;
		return 0;
	}

	if (write)
		ret = memcpy_from_guest(gaddr, st->iobuf, &len);
	else
		ret = memcpy_to_guest(gaddr, st->iobuf, &len);

	if (ret)
		st->sch_status = SC_PROT_CHK;

	st->pos = 0;
	st->rem = len;

	return 0;
}

static void __get_ccw(struct virt_sys *sys, struct spdev_state *st, struct ccw *ccw)
{
	struct ccw0 rawccw;
	u64 len;
	int ret;

	if ((!st->f && (st->addr & 0xff000000)) || (st->f && (st->addr & 0x80000000)) ||
	    (st->addr & 0x00000007)) {
		st->sch_status = SC_PROG_CHK;
		return;
	}

	len = sizeof(struct ccw);
	ret = memcpy_from_guest(st->addr, &rawccw, &len);
	if (ret) {
		con_printf(sys->con, "failed to fetch ccw %d \n", ret);
		st->sch_status = SC_PROT_CHK;
		return;
	}

	if (!st->f)
		ccw0_to_ccw1(ccw, &rawccw);
	else
		memcpy(ccw, &rawccw, sizeof(struct ccw));
}

/*
 * This fuction behaves as a control unit as described in the principles of
 * operation.  It uses a function pointer to "send the command to the
 * device."
 */
int spool_exec(struct virt_sys *sys, struct virt_device *vdev)
{
	struct spdev_state st;
	struct page *pages;
	struct ccw ccw;

	int cnt = 0;

	pages = alloc_pages(4, ZONE_NORMAL); /* get 64k buf */
	if (!pages) {
		con_printf(sys->con, "out of memory: could not allocate "
			   "channel buffer (64kB)\n");
		return -ENOMEM;
	}

	st.iobuf      = page_to_addr(pages);
	st.dev_status = 0;
	st.sch_status = 0;
	st.cd         = 0;
	st.rem        = 0;
	st.pos        = 0;
	st.tic2tic    = 0;

	mutex_lock(&vdev->lock);
	vdev->scsw.ac &= ~AC_START;
	vdev->scsw.ac |= AC_SCH_ACT | AC_DEV_ACT;

	st.f    = vdev->orb.f;
	st.addr = vdev->orb.addr;
	st.ops  = vdev->u.spool.ops;

	mutex_unlock(&vdev->lock);

	con_printf(sys->con, "spool_exec for %04x (%08x), format-%d\n",
		   vdev->pmcw.dev_num, vdev->sch, st.f);

	for(;;) {
		con_printf(sys->con, "CCW %3d @%08x, ", cnt++, st.addr);

		/* ORB must contain a valid address */
		__get_ccw(sys, &st, &ccw);

		st.addr += 8;

		if (st.sch_status)
			goto out; /* in case __get_ccw failed */

		con_printf(sys->con, "%02x %02x %04x %08x ",
			   ccw.cmd, ccw.flags, ccw.count, ccw.addr);

		/* handle TICs */
		if (ccw.cmd == CCW_CMD_TIC) {
			if (st.tic2tic)
				goto prog_check;

			if (st.f && (ccw.flags || ccw.count))
				goto prog_check;

			/* Note: we'll end up checking the address next interation */
			st.addr = ccw.addr;
			st.tic2tic = 1;
			continue;
		}

		/* reset the tic-to-tic flag */
		st.tic2tic = 0;

		//////////////////////////////////////////////////////

		/* handle invalid commands */
		if (!st.cd && ((ccw.cmd & 0x0f) == 0x00))
			goto prog_check;

		/*
		 * Invalid Count, Format 0: A CCW, which is other
		 *   than a CCW specifying transfer in channel, contains
		 *   zeros in bit positions 48-63.
		 */
		if (!st.f && !ccw.count)
			goto prog_check;

		/*
		 * Invalid Count, Format 1: A CCW that specifies
		 *   data chaining or a CCW fetched while data chaining
		 *   contains zeros in bit positions 16-31.
		 */
		if (st.f && st.cd && !ccw.count)
			goto prog_check;

		/* The Data address must be valid */
		if (st.f && (ccw.addr & 0x80000000))
			goto prog_check;

		/* FIXME */
		if (ccw.flags & (CCW_FLAG_SKP | CCW_FLAG_PCI | CCW_FLAG_IDA |
				 CCW_FLAG_S | CCW_FLAG_MIDA | CCW_FLAG_CD)) {
			con_printf(sys->con, "unsupported ccw flag ");
			goto prog_check;
		}

		/*
		 * Write operations need to get data from storage
		 */
		if (IS_CCW_WRITE(ccw.cmd)) {
			if (__xfer_storage(sys, vdev, &st, &ccw, 1))
				break;
		}

		/*
		 * Great!  The channel decoded the CCW properly, now it's
		 * time to tell the device to execute the operation.
		 */

		if (st.ops && st.ops->f[ccw.cmd & 0xf])
			st.ops->f[ccw.cmd & 0xf](sys, vdev, &ccw, &st);
		else
			spooldev_cmdrej(sys, vdev, &st);

		if (st.dev_status != (SC_STATUS_CE | SC_STATUS_DE))
			break; /* command failed, stop chanining */

		/*
		 * Read operations need to get data to storage
		 */
		if (IS_CCW_READ(ccw.cmd)) {
			if (__xfer_storage(sys, vdev, &st, &ccw, 0))
				break;
		}

		con_printf(sys->con, "OK\n");

		if ((ccw.flags & (CCW_FLAG_CD | CCW_FLAG_CC)) == 0)
			break; /* end of chain */

		st.cd = (ccw.flags & CCW_FLAG_CD) != 0;

		st.addr = (st.addr + 8) & 0x7fffffff;
	}

out:
	if ((st.dev_status != (SC_STATUS_CE | SC_STATUS_DE)) || st.sch_status)
		con_printf(sys->con, "FAILED\n");

	mutex_lock(&vdev->lock);
	vdev->scsw.dev_status = st.dev_status;
	vdev->scsw.sch_status = st.sch_status;
	vdev->scsw.addr = st.addr;
	vdev->scsw.f = st.f;
	mutex_unlock(&vdev->lock);

	free_pages(st.iobuf, 4);

	return 0;

prog_check:
	st.sch_status = SC_PROG_CHK;
	goto out;
}
