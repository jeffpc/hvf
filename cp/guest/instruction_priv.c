/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <list.h>
#include <sched.h>
#include <vdevice.h>
#include <vcpu.h>
#include <shell.h>

static int handle_msch(struct virt_sys *sys)
{
	struct virt_cpu *cpu = sys->cpu;

	u64 r1 = __guest_gpr(cpu, 1);
	u64 addr = RAW_S_1(cpu);

	u64 len;
	struct schib gschib;
	struct virt_device *vdev, *vdev_cur;
	int ret = 0;

	/* sch number must be: X'0001____' */
	if ((r1 & 0xffff0000) != 0x00010000) {
		queue_prog_exception(sys, PROG_OPERAND, r1);
		goto out;
	}

	/* schib must be word-aligned */
	if (addr & 0x3) {
		queue_prog_exception(sys, PROG_SPEC, addr);
		goto out;
	}

	/* find the virtual device */
	vdev = NULL;
	for_each_vdev(sys, vdev_cur) {
		if (vdev_cur->sch == (u32) r1) {
			vdev = vdev_cur;
			break;
		}
	}

	/* There's no virtual device with this sch number; CC=3 */
	if (!vdev) {
		cpu->sie_cb.gpsw.cc = 3;
		goto out;
	}

	/* copy the SCHIB from the guest */
	len = sizeof(struct schib);
	ret = memcpy_from_guest(addr, &gschib, &len);
	if (ret) {
		if (ret != -EFAULT)
			goto out;

		ret = 0;
		queue_prog_exception(sys, PROG_ADDR, addr);
		goto out;
	}

	mutex_lock(&vdev->lock);

	/*
	 * Condition code 1 is set, and no other action is taken, when the
	 *   subchannel is status pending. (See “Status Control (SC)” on
	 *   page 16-16.)
	 */
	if (vdev->scsw.sc & SC_STATUS) {
		cpu->sie_cb.gpsw.cc = 1;
		goto out_unlock;
	}

	/*
	 * Condition code 2 is set, and no other action is taken, when a
	 *   clear, halt, or start function is in progress at the
	 *   subchannel.  (See “Function Control (FC)” on page 16-12.)
	 */
	if (vdev->scsw.fc & (FC_START | FC_HALT | FC_CLEAR)) {
		cpu->sie_cb.gpsw.cc = 2;
		goto out_unlock;
	}

	/*
	 * The channel-subsystem operations that may be influenced due to
	 * placement of SCHIB information in the subchannel are:
	 *
	 * • I/O processing (E field)
	 * • Interruption processing (interruption parameter and ISC field)
	 * • Path management (D, LPM, and POM fields)
	 * • Monitoring and address-limit checking (measurement-block index,
	 *   LM, and MM fields)
	 * • Measurement-block-format control (F field)
	 * • Extended-measurement-word-mode enable (X field)
	 * • Concurrent-sense facility (S field)
	 * • Measurement-block address (MBA)
	*/

	if (!gschib.pmcw.v)
		goto out_cc0;

	vdev->pmcw.interrupt_param = gschib.pmcw.interrupt_param;
	vdev->pmcw.e   = gschib.pmcw.e;
	vdev->pmcw.isc = gschib.pmcw.isc;
	vdev->pmcw.d   = gschib.pmcw.d;
	vdev->pmcw.lpm = gschib.pmcw.lpm;
	vdev->pmcw.pom = gschib.pmcw.pom;
	vdev->pmcw.lm  = gschib.pmcw.lm;
	vdev->pmcw.mm  = gschib.pmcw.mm;
	vdev->pmcw.f   = gschib.pmcw.f;
	vdev->pmcw.x   = gschib.pmcw.x;
	vdev->pmcw.s   = gschib.pmcw.s;
	vdev->pmcw.mbi = gschib.pmcw.mbi;

	FIXME("save measurement-block address");

out_cc0:
	cpu->sie_cb.gpsw.cc = 0;

out_unlock:
	mutex_unlock(&vdev->lock);

out:
	return ret;
}

static int handle_ssch(struct virt_sys *sys)
{
	struct virt_cpu *cpu = sys->cpu;
	u64 len;

	u64 r1   = __guest_gpr(cpu, 1);
	u64 addr = RAW_S_1(cpu);

	struct orb gorb;
	struct virt_device *vdev, *vdev_cur;
	enum directory_vdevtype vtype;
	int ret = 0;

	/* sch number must be: X'0001____' */
	if ((r1 & 0xffff0000) != 0x00010000) {
		queue_prog_exception(sys, PROG_OPERAND, r1);
		goto out;
	}

	/* schib must be word-aligned */
	if (addr & 0x3) {
		queue_prog_exception(sys, PROG_SPEC, addr);
		goto out;
	}

	/* find the virtual device */
	vdev = NULL;
	for_each_vdev(sys, vdev_cur) {
		if (vdev_cur->sch == (u32) r1) {
			vdev = vdev_cur;
			break;
		}
	}

	/* There's no virtual device with this sch number; CC=3 */
	if (!vdev) {
		cpu->sie_cb.gpsw.cc = 3;
		goto out;
	}

	/* copy the ORB from the guest */
	len = sizeof(struct orb);
	ret = memcpy_from_guest(addr, &gorb, &len);
	if (ret) {
		if (ret == -EFAULT) {
			ret = 0;
			queue_prog_exception(sys, PROG_ADDR, addr);
		}

		goto out;
	}

	mutex_lock(&vdev->lock);

	/*
	 * Condition code 1 is set, and no other action is taken, when the
	 *   subchannel is status pending. (See “Status Control (SC)” on
	 *   page 16-16.)
	 */
	if (vdev->scsw.sc & SC_STATUS) {
		mutex_unlock(&vdev->lock);
		cpu->sie_cb.gpsw.cc = 1;
		goto out;
	}

	/*
	 * Condition code 2 is set, and no other action is taken, when a
	 *   clear, halt, or start function is in progress at the
	 *   subchannel.  (See “Function Control (FC)” on page 16-12.)
	 */
	if (vdev->scsw.fc & (FC_START | FC_HALT | FC_CLEAR)) {
		mutex_unlock(&vdev->lock);
		cpu->sie_cb.gpsw.cc = 2;
		goto out;
	}

	/*
	 * Let's set up the subchannel status for the I/O
	 */
	memset(&vdev->scsw, 0, sizeof(struct scsw));
	vdev->scsw.fc = FC_START;
	vdev->scsw.ac = AC_START;
	vdev->pmcw.interrupt_param = gorb.param;
	memcpy(&vdev->orb, &gorb, sizeof(struct orb));

	vtype = vdev->vtype;

	mutex_unlock(&vdev->lock);

	switch (vtype) {
		case VDEV_CONS:
		case VDEV_SPOOL:
			ret = spool_exec(sys, vdev);
			break;

		default:
			goto stop;
	}

	cpu->sie_cb.gpsw.cc = 0;

out:
	return ret;

stop:
	cpu->state = GUEST_STOPPED;
	con_printf(sys->con, "SSCH handler - CPU STOPPED\n");

	return 0;
}

static int handle_stsch(struct virt_sys *sys)
{
	struct virt_cpu *cpu = sys->cpu;

	u64 r1   = __guest_gpr(cpu, 1);
	u64 addr = RAW_S_1(cpu);
	u64 taddr;
	u64 len;

	struct virt_device *vdev, *vdev_cur;
	int ret = 0;

	/* sch number must be: X'0001____' */
	if ((r1 & 0xffff0000) != 0x00010000) {
		queue_prog_exception(sys, PROG_OPERAND, r1);
		goto out;
	}

	/* schib must be word-aligned */
	if (addr & 0x3) {
		queue_prog_exception(sys, PROG_SPEC, addr);
		goto out;
	}

	/* find the virtual device */
	vdev = NULL;
	for_each_vdev(sys, vdev_cur) {
		if (vdev_cur->sch == (u32) r1) {
			vdev = vdev_cur;
			break;
		}
	}

	/* There's no virtual device with this sch number; CC=3 */
	if (!vdev) {
		cpu->sie_cb.gpsw.cc = 3;
		goto out;
	}

	mutex_lock(&vdev->lock);

	/* copy! */
	taddr = addr + offsetof(struct schib, pmcw);
	len = sizeof(struct pmcw);
	ret = memcpy_to_guest(taddr, &vdev->pmcw, &len);
	if (ret) {
		if (ret == -EFAULT) {
			ret = 0;
			queue_prog_exception(sys, PROG_ADDR, taddr);
		}

		mutex_unlock(&vdev->lock);
		goto out;
	}

	taddr = addr + offsetof(struct schib, scsw);
	len = sizeof(struct scsw);
	ret = memcpy_to_guest(taddr, &vdev->scsw, &len);
	if (ret) {
		if (ret == -EFAULT) {
			ret = 0;
			queue_prog_exception(sys, PROG_ADDR, taddr);
		}

		mutex_unlock(&vdev->lock);
		goto out;
	}

	mutex_unlock(&vdev->lock);

	/* CC: 0 */
	cpu->sie_cb.gpsw.cc = 0;

out:
	return ret;
}

static int handle_tsch(struct virt_sys *sys)
{
	struct virt_cpu *cpu = sys->cpu;

	u64 r1   = __guest_gpr(cpu, 1);
	u64 addr = RAW_S_1(cpu);

	struct irb irb;
	struct virt_device *vdev, *vdev_cur;
	int ret = 0;
	int cc;

	/* sch number must be: X'0001____' */
	if ((r1 & 0xffff0000) != 0x00010000) {
		queue_prog_exception(sys, PROG_OPERAND, r1);
		goto out;
	}

	/* schib must be word-aligned */
	if (addr & 0x3) {
		queue_prog_exception(sys, PROG_SPEC, addr);
		goto out;
	}

	/* find the virtual device */
	vdev = NULL;
	for_each_vdev(sys, vdev_cur) {
		if (vdev_cur->sch == (u32) r1) {
			vdev = vdev_cur;
			break;
		}
	}

	/* There's no virtual device with this sch number; CC=3 */
	if (!vdev) {
		cpu->sie_cb.gpsw.cc = 3;
		goto out;
	}

	memset(&irb, 0, sizeof(struct irb));

	mutex_lock(&vdev->lock);

	memcpy(&irb.scsw, &vdev->scsw, sizeof(struct scsw));

	if (vdev->scsw.sc & SC_STATUS) {
		u32 fc, ac, sc;

		fc = vdev->scsw.fc;
		ac = vdev->scsw.ac;
		sc = vdev->scsw.sc;

		if (!(vdev->scsw.sc & SC_INTERMED) ||
		    ((vdev->scsw.fc & FC_HALT) && (vdev->scsw.ac & AC_SUSP)))
			fc = 0;

		if (!(vdev->scsw.sc & SC_INTERMED))
			ac &= ~(AC_RESUME | AC_START | AC_HALT | AC_CLEAR);
		else if ((vdev->scsw.ac & AC_SUSP) && (vdev->scsw.fc & FC_START)) {
			if (vdev->scsw.fc & FC_HALT)
				ac &= ~(AC_RESUME | AC_START | AC_HALT | AC_CLEAR);
			else
				ac &= ~AC_RESUME;
		}

		sc &= ~SC_STATUS;

		FIXME("N-condition");

		vdev->scsw.fc = fc;
		vdev->scsw.ac = ac;
		vdev->scsw.sc = sc;

		FIXME("dequeue I/O INT");
		cc = 0;
	} else {
		cc = 1;
	}

	mutex_unlock(&vdev->lock);

	cpu->sie_cb.gpsw.cc = cc;

out:
	return ret;
}

static const intercept_handler_t instruction_priv_funcs[256] = {
	[0x32] = handle_msch,
	[0x33] = handle_ssch,
	[0x34] = handle_stsch,
	[0x35] = handle_tsch,
};

int handle_instruction_priv(struct virt_sys *sys)
{
	struct virt_cpu *cpu = sys->cpu;
	intercept_handler_t h;
	int err = -EINVAL;

	h = instruction_priv_funcs[cpu->sie_cb.ipa & 0xff];
	if (h)
		err = h(sys);

	return err;
}

