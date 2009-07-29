#include <list.h>
#include <sched.h>
#include <cp.h>
#include <vdevice.h>
#include <vcpu.h>

static int handle_msch(struct virt_sys *sys)
{
	con_printf(sys->con, "MSCH handler\n");
	return -ENOMEM;
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
	struct virt_device *vdev;
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
	list_for_each_entry(vdev, &sys->virt_devs, devices) {
		if (vdev->sch == (u32) r1)
			break;
	}

	/* There's no virtual device with this sch number; CC=3 */
	if (vdev == container_of(&sys->virt_devs, struct virt_device, devices)) {
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

