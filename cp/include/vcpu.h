/*
 * (C) Copyright 2007-2019  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __VCPU_H
#define __VCPU_H

#include <list.h>
#include <circbuf.h>
#include <console.h>
#include <mutex.h>

/*
 * saved registers for guests
 *
 * NOTE: some registers are saved in the SIE control block!
 */
struct guest_regs {
	u64 gpr[16];
	u32 ar[16];
	u64 fpr[64];
	u32 fpcr;
};

/*
 * These states mirror those described in chapter 4 of SA22-7832-06
 */
enum virt_cpustate {
	GUEST_STOPPED = 0,
	GUEST_OPERATING,
	GUEST_LOAD,
	GUEST_CHECKSTOP,
};

#include <sie.h>

struct vio_int {
	struct list_head list;

	u32 ssid;
	u32 param;
	u32 intid;
};

struct virt_cpu {
	/* the SIE control block is picky about alignment */
	struct sie_cb sie_cb;

	struct guest_regs regs;
	u64 cpuid;

	enum virt_cpustate state;

	mutex_t int_lock;
	struct list_head int_io[8];	/* I/O interrupts */
};

struct virt_cons {
	struct device *dev;		/* the real device */

	struct spool_file *wlines;
	struct spool_file *rlines;

	u8 *bigbuf;
};

#define NUM_CRWS	32
struct virt_sys {
	struct task *task;		/* the virtual CPU task */
	struct virt_cpu *cpu;		/* the virtual CPU state */

	struct user *directory;		/* the directory information */

	struct virt_cons console;	/* the console */
	struct virt_cons *con;		/* convenience pointer to the
					   console struct */

	bool internal;			/* internal guest */
	bool print_ts;			/* print timestamps */
	bool print_name;		/* print guest name */

	struct list_head guest_pages;	/* list of guest pages */
	struct list_head online_users;	/* list of online users */

	mutex_t virt_devs_lock;
	struct list_head virt_devs;	/* list of guest virtual devs */
	CIRCBUF(crws, NUM_CRWS, struct crw);

	struct address_space as;	/* the guest storage */
};

/*****************************************************************************/
/* Guest exception queuing                                                   */

enum PROG_EXCEPTION {
	PROG_OPERAND		= 0x0001,
	PROG_PRIV		= 0x0002,
	PROG_EXEC		= 0x0003,
	PROG_PROT		= 0x0004,
	PROG_ADDR		= 0x0005,
	PROG_SPEC		= 0x0006,
	PROG_DATA		= 0x0007,
};

extern void queue_prog_exception(struct virt_sys *sys, enum PROG_EXCEPTION type, u64 param);
extern void queue_io_interrupt(struct virt_sys *sys, u32 ssid, u32 param, u32 a, u32 isc);

/*****************************************************************************/
/* Guest register reading & address calculation                              */

static inline u64 __guest_gpr(struct virt_cpu *cpu, int gpr)
{
	u64 ret = cpu->regs.gpr[gpr];

	if (!(atomic_read(&cpu->sie_cb.cpuflags) & CPUSTAT_ZARCH))
		ret &= 0xffffffffULL;

	return ret;
}

static inline u64 __guest_addr(struct virt_cpu *cpu, u64 disp, int x, int b)
{
	u64 mask = 0;

	switch((cpu->sie_cb.gpsw.ea << 1) | cpu->sie_cb.gpsw.ba) {
		case 3:
			mask = ~0;
			break;
		case 1:
			mask = 0x7fffffff;
			break;
		case 0:
			mask = 0x00ffffff;
			break;
		default:
			BUG();
			break;
	}

	return (disp +
		(x ? __guest_gpr(cpu, x) : 0) +
		(b ? __guest_gpr(cpu, b) : 0)) & mask;
}

/*****************************************************************************/
/* SIE Interception Param parsing & instruction decode                       */

#define IP_TO_RAW(sie)		((((u64)(sie).ipa) << 32) | ((u64)(sie).ipb))

static inline u64 RAW_S_1(struct virt_cpu *cpu)
{
	u64 raw = IP_TO_RAW(cpu->sie_cb);

	return __guest_addr(cpu,
			    (raw >> 16) & 0xfff,
			    (raw) >> 28 & 0xf,
			    0);
}

/*****************************************************************************/
/* All the different ways to reset the system */
extern void guest_power_on_reset(struct virt_sys *sys);
extern void guest_system_reset_normal(struct virt_sys *sys);
extern void guest_system_reset_clear(struct virt_sys *sys);
extern void guest_load_normal(struct virt_sys *sys);
extern void guest_load_clear(struct virt_sys *sys);

extern void run_guest(struct virt_sys *sys);
extern void handle_interception(struct virt_sys *sys);

extern int handle_instruction(struct virt_sys *sys);
extern int handle_instruction_priv(struct virt_sys *sys);

typedef int (*intercept_handler_t)(struct virt_sys *sys);

#endif
