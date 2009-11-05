#ifndef __SIE_H
#define __SIE_H

/*
 * Taken from linux/arch/s390/include/asm/kvm_host.h
 */

#include <atomic.h>

/*
 * Constants for sie_cb->cpuflags
 */
#define CPUSTAT_HOST		0x80000000
#define CPUSTAT_WAIT		0x10000000
#define CPUSTAT_ECALL_PEND	0x08000000
#define CPUSTAT_STOP_INT	0x04000000
#define CPUSTAT_IO_INT		0x02000000
#define CPUSTAT_EXT_INT		0x01000000
#define CPUSTAT_RUNNING		0x00800000
#define CPUSTAT_RETAINED	0x00400000
#define CPUSTAT_TIMING_SUB	0x00020000
#define CPUSTAT_SIE_SUB		0x00010000
#define CPUSTAT_RRF		0x00008000
#define CPUSTAT_SLSV		0x00004000
#define CPUSTAT_SLSR		0x00002000
#define CPUSTAT_ZARCH		0x00000800
#define CPUSTAT_MCDS		0x00000100
#define CPUSTAT_SM		0x00000080
#define CPUSTAT_G		0x00000008
#define CPUSTAT_J		0x00000002
#define CPUSTAT_P		0x00000001

struct sie_cb {
	atomic_t	cpuflags;		/* 0x0000 */
	u32		prefix;			/* 0x0004 */
	u8		reserved8[32];		/* 0x0008 */
	u64		cputm;			/* 0x0028 */
	u64		ckc;			/* 0x0030 */
	u64		epoch;			/* 0x0038 */
	u8		reserved40[4];		/* 0x0040 */
#define LCTL_CR0 0x8000
	u16		lctl;			/* 0x0044 */
	s16		icpua;			/* 0x0046 */
	u32		ictl;			/* 0x0048 */
	u32		eca;			/* 0x004c */
	u8		icptcode;		/* 0x0050 */
	u8		reserved51;		/* 0x0051 */
	u16		ihcpu;			/* 0x0052 */
	u8		reserved54[2];		/* 0x0054 */
	u16		ipa;			/* 0x0056 */
	u32		ipb;			/* 0x0058 */
	u32		scaoh;			/* 0x005c */
	u8		reserved60;		/* 0x0060 */
	u8		ecb;			/* 0x0061 */
	u8		reserved62[2];		/* 0x0062 */
	u32		scaol;			/* 0x0064 */
	u8		reserved68[4];		/* 0x0068 */
	u32		todpr;			/* 0x006c */
	u8		reserved70[16];		/* 0x0070 */
	u64		gmsor;			/* 0x0080 */
	u64		gmslm;			/* 0x0088 */
	struct psw	gpsw;			/* 0x0090 */
	u64		gg14;			/* 0x00a0 */
	u64		gg15;			/* 0x00a8 */
	u8		reservedb0[30];		/* 0x00b0 */
	u16		iprcc;			/* 0x00ce */
	u8		reservedd0[48];		/* 0x00d0 */
	u64		gcr[16];		/* 0x0100 */
	u64		gbea;			/* 0x0180 */
	u8		reserved188[120];	/* 0x0188 */
} __attribute__((aligned(256),packed));

#define VCPU_ZARCH(vcpu)	(atomic_read(&((vcpu)->sie_cb.cpuflags)) & CPUSTAT_ZARCH)

#endif
