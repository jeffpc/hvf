/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <directory.h>
#include <sched.h>
#include <dat.h>
#include <shell.h>

#define RESET_CPU			0x000001
#define SET_ESA390			0x000002
#define RESET_PSW			0x000004
#define RESET_PREFIX			0x000008
#define RESET_CPU_TIMER			0x000010
#define RESET_CLK_COMP			0x000020
#define RESET_TOD_PROG_REG		0x000040
#define RESET_CR			0x000080
#define RESET_BREAK_EV_ADDR		0x000100
#define RESET_FPCR			0x000200
#define RESET_AR			0x000400
#define RESET_GPR			0x000800
#define RESET_FPR			0x001000
#define RESET_STORAGE_KEYS		0x002000
#define RESET_STORAGE			0x004000
#define RESET_NONVOL_STORAGE		0x008000
#define RESET_EXPANDED_STORAGE		0x010000
#define RESET_TOD			0x020000
#define RESET_TOD_STEER			0x040000
#define RESET_FLOATING_INTERRUPTIONS	0x080000
#define RESET_IO			0x100000
#define RESET_PLO_LOCKS			0x200000
#define __RESET_PLO_LOCKS_PRESERVE	0x400000
#define RESET_PLO_LOCKS_PRESERVE	(RESET_PLO_LOCKS | \
					 __RESET_PLO_LOCKS_PRESERVE)

/*
 * These define all the different ways of reseting the system...to save us
 * typing later on :)
 */
#define SUBSYSTEM_RESET_FLAGS	(RESET_FLOATING_INTERRUPTIONS | RESET_IO)
#define CPU_RESET_FLAGS		(RESET_CPU)
#define INIT_CPU_RESET_FLAGS	(RESET_CPU | SET_ESA390 | RESET_PSW | \
				 RESET_PREFIX | RESET_CPU_TIMER | \
				 RESET_CLK_COMP | RESET_TOD_PROG_REG | \
				 RESET_CR | RESET_BREAK_EV_ADDR | \
				 RESET_FPCR)
#define CLEAR_RESET_FLAGS	(RESET_CPU | SET_ESA390 | RESET_PSW | \
				 RESET_PREFIX | RESET_CPU_TIMER | \
				 RESET_CLK_COMP | RESET_TOD_PROG_REG | \
				 RESET_CR | RESET_BREAK_EV_ADDR | RESET_FPCR | \
				 RESET_AR | RESET_GPR | RESET_FPR | \
				 RESET_STORAGE_KEYS | RESET_STORAGE | \
				 RESET_NONVOL_STORAGE | RESET_PLO_LOCKS | \
				 RESET_FLOATING_INTERRUPTIONS | RESET_IO)
#define POWER_ON_RESET_FLAGS	(RESET_CPU | SET_ESA390 | RESET_PSW | \
				 RESET_PREFIX | RESET_CPU_TIMER | \
				 RESET_CLK_COMP | RESET_TOD_PROG_REG | \
				 RESET_CR | RESET_BREAK_EV_ADDR | RESET_FPCR | \
				 RESET_AR | RESET_GPR | RESET_FPR | \
				 RESET_STORAGE_KEYS | RESET_STORAGE | \
				 RESET_EXPANDED_STORAGE | RESET_TOD | \
				 RESET_TOD_STEER | RESET_PLO_LOCKS_PRESERVE | \
				 RESET_FLOATING_INTERRUPTIONS | RESET_IO)

/*
 * Reset TODO:
 *  - handle Captured-z/Architecture-PSW register
 */
static void __perform_cpu_reset(struct virt_sys *sys, int flags)
{
	struct virt_cpu *cpu = sys->task->cpu;

	if (flags & RESET_CPU) {
		if (flags & SET_ESA390) {
			/* FIXME: set the arch mode to ESA/390 */
		}

		/*
		 * FIXME: clear interruptions:
		 *  - PROG
		 *  - SVC
		 *  - local EXT (floating EXT are NOT cleared)
		 *  - MCHECK (floating are NOT cleared)
		 */

		cpu->state = GUEST_STOPPED;
	}

	if (flags & RESET_PSW)
		memset(&cpu->sie_cb.gpsw, 0, sizeof(struct psw));

	if (flags & RESET_PREFIX)
		cpu->sie_cb.prefix = 0;

	if (flags & RESET_CPU_TIMER) {
		/* FIXME */
	}

	if (flags & RESET_CLK_COMP) {
		/* FIXME */
	}

	if (flags & RESET_TOD_PROG_REG) {
		/* FIXME */
	}

	if (flags & RESET_CR) {
		memset(cpu->sie_cb.gcr, 0, 16*sizeof(u64));
		cpu->sie_cb.gcr[0]  = 0xE0UL;
		cpu->sie_cb.gcr[14] = 0xC2000000UL;
	}

	if (flags & RESET_BREAK_EV_ADDR) {
		/* FIXME: initialize to 0x1 */
	}

	if (flags & RESET_FPCR)
		cpu->regs.fpcr = 0;

	if (flags & RESET_AR)
		memset(cpu->regs.ar, 0, 16*sizeof(u32));

	if (flags & RESET_GPR)
		memset(cpu->regs.gpr, 0, 16*sizeof(u64));

	if (flags & RESET_FPR)
		memset(cpu->regs.fpr, 0, 16*sizeof(u64));

	if (flags & RESET_STORAGE_KEYS) {
	}

	if (flags & RESET_STORAGE) {
		struct page *p;

		list_for_each_entry(p, &sys->guest_pages, guest)
			memset(page_to_addr(p), 0, PAGE_SIZE);
	}

	if (flags & RESET_NONVOL_STORAGE) {
	}

	if (flags & RESET_EXPANDED_STORAGE) {
	}

	if (flags & RESET_TOD) {
	}

	if (flags & RESET_TOD_STEER) {
	}

	if (flags & RESET_PLO_LOCKS) {
		/*
		 * TODO: if RESET_PLO_LOCKS_PRESERVE is set, don't reset
		 * locks held by powered on CPUS
		 */
	}
}

static void __perform_noncpu_reset(struct virt_sys *sys, int flags)
{
	if (flags & RESET_FLOATING_INTERRUPTIONS) {
	}

	if (flags & RESET_IO) {
	}
}

/**************/

void guest_power_on_reset(struct virt_sys *sys)
{
	__perform_cpu_reset(sys, POWER_ON_RESET_FLAGS);
	__perform_noncpu_reset(sys, POWER_ON_RESET_FLAGS);
}

void guest_system_reset_normal(struct virt_sys *sys)
{
	__perform_cpu_reset(sys, CPU_RESET_FLAGS);

	/*
	 * TODO: once we have SMP guests, all other cpus should get a
	 * CPU_RESET_FLAGS as well.
	 */

	__perform_noncpu_reset(sys, SUBSYSTEM_RESET_FLAGS);
}

void guest_system_reset_clear(struct virt_sys *sys)
{
	__perform_cpu_reset(sys, CLEAR_RESET_FLAGS);

	/*
	 * TODO: once we have SMP guests, all other cpus should get a
	 * CLEAR_RESET_FLAGS as well.
	 */

	__perform_noncpu_reset(sys, CLEAR_RESET_FLAGS);
}

void guest_load_normal(struct virt_sys *sys)
{
	__perform_cpu_reset(sys, INIT_CPU_RESET_FLAGS);

	/*
	 * TODO: once we have SMP guests, all other cpus should get a
	 * CPU_RESET_FLAGS.
	 */

	__perform_noncpu_reset(sys, SUBSYSTEM_RESET_FLAGS);
}

void guest_load_clear(struct virt_sys *sys)
{
	__perform_cpu_reset(sys, CLEAR_RESET_FLAGS);

	/*
	 * TODO: once we have SMP guests, all other cpus should get a
	 * CLEAR_RESET_FLAGS as well.
	 */

	__perform_noncpu_reset(sys, CLEAR_RESET_FLAGS);
}
