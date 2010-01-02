/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __VCPU_H
#define __VCPU_H

#include <sched.h>

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
	u64 mask;

	if (cpu->sie_cb.gpsw.ea && cpu->sie_cb.gpsw.ba)
		mask = ~0;
	else if (!cpu->sie_cb.gpsw.ea && cpu->sie_cb.gpsw.ba)
		mask = 0x7fffffff;
	else if (!cpu->sie_cb.gpsw.ea && !cpu->sie_cb.gpsw.ba)
		mask = 0x00ffffff;
	else
		BUG();

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

#endif
