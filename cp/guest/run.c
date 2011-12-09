/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <directory.h>
#include <sched.h>
#include <dat.h>
#include <vcpu.h>
#include <slab.h>

static void __do_psw_swap(struct virt_cpu *cpu, u8 *gpsa, u64 old, u64 new,
			  int len)
{
	memcpy(gpsa + old, &cpu->sie_cb.gpsw, len);
	memcpy(&cpu->sie_cb.gpsw, gpsa + new, len);
}

static int __io_int(struct virt_sys *sys)
{
	struct virt_cpu *cpu = sys->cpu;
	struct vio_int *ioint;
	u64 cr6;
	u64 phy;
	int ret;
	int i;

	/* are I/O interrupts enabled? */
	if (!cpu->sie_cb.gpsw.io)
		return 0;

	cr6 = cpu->sie_cb.gcr[6];

	mutex_lock(&cpu->int_lock);
	/* for each subclass, check that... */
	for(i=0; i<8; i++) {
		/* ...the subclass is enabled */
		if (!(cr6 & (0x80000000 >> i)))
			continue;

		/* ...there are interrupts of that subclass */
		if (list_empty(&cpu->int_io[i]))
			continue;

		/* there is an interruption that we can perform */
		ioint = list_first_entry(&cpu->int_io[i], struct vio_int, list);
		list_del(&ioint->list);

		mutex_unlock(&cpu->int_lock);

		con_printf(sys->con, "Time for I/O int... isc:%d "
			   "%08x.%08x.%08x\n", i, ioint->ssid,
			   ioint->param, ioint->intid);

		/* get the guest's first page (PSA) */
		ret = virt2phy_current(0, &phy);
		if (ret) {
			con_printf(sys->con, "Failed to queue up I/O "
				   "interruption: %d (%s)\n", ret,
				   errstrings[-ret]);
			cpu->state = GUEST_STOPPED;

			return 0;
		}

		/* do the PSW swap */
		if (VCPU_ZARCH(cpu))
			__do_psw_swap(cpu, (void*) phy, 0x170, 0x1f0, 16);
		else
			__do_psw_swap(cpu, (void*) phy, 56, 120, 8);

		*((u32*) (phy + 184)) = ioint->ssid;
		*((u32*) (phy + 188)) = ioint->param;
		*((u32*) (phy + 192)) = ioint->intid;

		free(ioint);

		return 1;
	}

	mutex_unlock(&cpu->int_lock);
	return 0;
}

/*
 * FIXME:
 * - issue any pending interruptions
 */
void run_guest(struct virt_sys *sys)
{
	struct psw *psw = &sys->cpu->sie_cb.gpsw;
	u64 save_gpr[16];

	if (__io_int(sys))
		goto go;

	if (psw->w) {
		if (!psw->io && !psw->ex && !psw->m) {
			con_printf(sys->con, "ENTERED CP DUE TO DISABLED WAIT\n");
			sys->cpu->state = GUEST_STOPPED;
			return;
		}

		/* wait state */
		schedule();
		return;
	}

go:
	/*
	 * FIXME: need to ->icptcode = 0;
	 */

	/*
	 * FIXME: load FPRs & FPCR
	 */

	/*
	 * IMPORTANT: We MUST keep a valid stack address in R15. This way,
	 * if SIE gets interrupted via an interrupt in the host, the
	 * scheduler can still get to the struct task pointer on the stack
	 */
	asm volatile(
		/* save current regs */
		"	stmg	0,15,%0\n"
		/* load the address of the guest state save area */
		"	lr	14,%1\n"
		/* load the address of the reg save area */
		"	la	15,%0\n"
		/* load guest's R0-R13 */
		"	lmg	0,13,%2(14)\n"
		/* SIE */
		"	sie	%3(14)\n"
		/* save guest's R0-R13 */
		"	stmg	0,13,%2(14)\n"
		/* restore all regs */
		"	lmg	0,15,0(15)\n"

	: /* output */
	  "+m" (save_gpr)
	: /* input */
	  "a" (sys->cpu),
	  "J" (offsetof(struct virt_cpu, regs.gpr)),
	  "J" (offsetof(struct virt_cpu, sie_cb))
	: /* clobbered */
	  "memory"
	);

	/*
	 * FIXME: store FPRs & FPCR
	 */

	handle_interception(sys);
}
