#include <list.h>
#include <sched.h>
#include <cp.h>
#include <vdevice.h>
#include <vcpu.h>

static int handle_msch(struct virt_sys *sys)
{
	struct virt_cpu *cpu = sys->task->cpu;

	u64 r1 = __guest_gpr(cpu, 1);
	u64 addr = RAW_S_1(cpu);

	struct schib *gschib;
	struct virt_device *vdev, *vdev_cur;
	int ret = 0;

	if ((PAGE_SIZE-(addr & PAGE_MASK)) < sizeof(struct schib)) {
		con_printf(sys->con, "The SCHIB crosses page boundary (%016llx; %lu)! CPU stopped\n",
			   addr, sizeof(struct schib));
		cpu->state = GUEST_STOPPED;
		goto out;
	}

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
	list_for_each_entry(vdev_cur, &sys->virt_devs, devices) {
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

	/* translate guest address to host address */
	ret = virt2phy_current(addr, &addr);
	if (ret) {
		if (ret != -EFAULT)
			goto out;

		ret = 0;
		queue_prog_exception(sys, PROG_ADDR, addr);
		goto out;
	}

	gschib = (struct schib*) addr;

	/*
	 * Condition code 1 is set, and no other action is taken, when the
	 *   subchannel is status pending. (See “Status Control (SC)” on
	 *   page 16-16.)
	 */
	if (vdev->scsw.sc & SC_STATUS) {
		cpu->sie_cb.gpsw.cc = 1;
		goto out;
	}

	/*
	 * Condition code 2 is set, and no other action is taken, when a
	 *   clear, halt, or start function is in progress at the
	 *   subchannel.  (See “Function Control (FC)” on page 16-12.)
	 */
	if (vdev->scsw.fc & (FC_START | FC_HALT | FC_CLEAR)) {
		cpu->sie_cb.gpsw.cc = 2;
		goto out;
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

	if (!gschib->pmcw.v)
		goto out_cc0;

	vdev->pmcw.interrupt_param = gschib->pmcw.interrupt_param;
	vdev->pmcw.e   = gschib->pmcw.e;
	vdev->pmcw.isc = gschib->pmcw.isc;
	vdev->pmcw.d   = gschib->pmcw.d;
	vdev->pmcw.lpm = gschib->pmcw.lpm;
	vdev->pmcw.pom = gschib->pmcw.pom;
	vdev->pmcw.lm  = gschib->pmcw.lm;
	vdev->pmcw.mm  = gschib->pmcw.mm;
	vdev->pmcw.f   = gschib->pmcw.f;
	vdev->pmcw.x   = gschib->pmcw.x;
	vdev->pmcw.s   = gschib->pmcw.s;
	vdev->pmcw.mbi = gschib->pmcw.mbi;
	/* FIXME: save measurement-block address */

out_cc0:
	cpu->sie_cb.gpsw.cc = 0;

out:
	return ret;
}

static int handle_ssch(struct virt_sys *sys)
{
	con_printf(sys->con, "SSCH handler\n");
	return -ENOMEM;
}

static int handle_stsch(struct virt_sys *sys)
{
	struct virt_cpu *cpu = sys->task->cpu;

	u64 r1   = __guest_gpr(cpu, 1);
	u64 addr = RAW_S_1(cpu);

	struct schib *gschib;
	struct virt_device *vdev, *vdev_cur;
	int ret = 0;

	if ((PAGE_SIZE-(addr & PAGE_MASK)) < sizeof(struct schib)) {
		con_printf(sys->con, "The SCHIB crosses page boundary (%016llx; %lu)! CPU stopped\n",
			   addr, sizeof(struct schib));
		cpu->state = GUEST_STOPPED;
		goto out;
	}

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
	list_for_each_entry(vdev_cur, &sys->virt_devs, devices) {
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

	/* translate guest address to host address */
	ret = virt2phy_current(addr, &addr);
	if (ret) {
		if (ret == -EFAULT) {
			ret = 0;
			queue_prog_exception(sys, PROG_ADDR, addr);
		}

		goto out;
	}

	gschib = (struct schib*) addr;

	/* copy! */
	memcpy(&gschib->pmcw, &vdev->pmcw, sizeof(struct pmcw));
	memcpy(&gschib->scsw, &vdev->scsw, sizeof(struct scsw));

	/* CC: 0 */
	cpu->sie_cb.gpsw.cc = 0;

out:
	return ret;
}

static const intercept_handler_t instruction_priv_funcs[256] = {
	[0x32] = handle_msch,
	[0x33] = handle_ssch,
	[0x34] = handle_stsch,
};

int handle_instruction_priv(struct virt_sys *sys)
{
	struct virt_cpu *cpu = sys->task->cpu;
	intercept_handler_t h;
	int err = -EINVAL;

	h = instruction_priv_funcs[cpu->sie_cb.ipa & 0xff];
	if (h)
		err = h(sys);

	return err;
}

