/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __INTERRUPT_H
#define __INTERRUPT_H

/*
 * I/O interruptions specific constants & structures
 */
struct io_int_code {
	u32 ssid;
	u32 param;
} __attribute__((packed));

struct mch_int_code {
	u8 sd:1,		/* System Damage */
	   pd:1,		/* Instruction-processing damage */
	   sr:1,		/* System recovery */
	   _pad0:1,
	   cd:1,		/* Timing-facility damage */
	   ed:1,		/* External damage */
	   _pad1:1,
	   dg:1;		/* Degradation */
	u8 w:1,			/* Warning */
	   cp:1,		/* Channel report pending */
	   sp:1,		/* Service-processor damage */
	   ck:1,		/* Channel-subsystem damage */
	   _pad2:2,
	   b:1,			/* Backed up */
	   _pad3:1;
	u8 se:1,		/* Storage error uncorrected */
	   sc:1,		/* Storage error corrected */
	   ke:1,		/* Storage-key error uncorrected */
	   ds:1,		/* Storage degradation */
	   wp:1,		/* PSW-MWP validity */
	   ms:1,		/* PSW mask and key validity */
	   pm:1,		/* PSW program-mask and condition-code validity */
	   ia:1;		/* PSW-instruction-address validity */
	u8 fa:1,		/* Failing-storage-address validity */
	   _pad4:1,
	   ec:1,		/* External-damage-code */
	   fp:1,		/* Floating-point-register validity */
	   gr:1,		/* General-register validity */
	   cr:1,		/* Control-register validity */
	   _pad5:1,
	   st:1;		/* Storage logical validity */
	u8 ie:1,		/* Indirect storage error */
	   ar:1,		/* Access-register validity */
	   da:1,		/* Delayed-access exception */
	   _pad6:5;
	u8 _pad7:2,
	   pr:1,		/* TOD-programable-register validity */
	   fc:1,		/* Floating-point-control-register validity */
	   ap:1,		/* Ancilary report */
	   _pad8:1,
	   ct:1,		/* CPU-timer validity */
	   cc:1;		/* Clock-comparator validity */
	u8 _pad9;
	u8 _pad10;
} __attribute__((packed));

#define PSA_INT_GPR	((u64*) 0x200)
#define PSA_TMP_PSW	((struct psw*) 0x280)

#define IO_INT_OLD_PSW	((void*) 0x170)
#define IO_INT_NEW_PSW	((void*) 0x1f0)
#define IO_INT_CODE	((struct io_int_code*) 0xb8)

#define EXT_INT_OLD_PSW	((void*) 0x130)
#define EXT_INT_NEW_PSW	((void*) 0x1b0)
#define EXT_INT_CODE	((u16*) 0x86)

#define SVC_INT_OLD_PSW ((void*) 0x140)
#define SVC_INT_NEW_PSW ((void*) 0x1c0)
#define SVC_INT_CODE	((u16*) 0x8a)

#define PGM_INT_OLD_PSW	((void*) 0x150)
#define PGM_INT_NEW_PSW ((void*) 0x1d0)
#define PGM_INT_ILC	((u8*) 0x8d)
#define PGM_INT_CODE	((u16*) 0x8e)

#define MCH_INT_OLD_PSW ((void*) 0x160)
#define MCH_INT_NEW_PSW ((void*) 0x1e0)
#define MCH_INT_CODE	((struct mch_int_code*) 232)

/*
 * Assembly stubs to call the C-handlers
 */
extern void IO_INT(void);
extern void EXT_INT(void);
extern void SVC_INT(void);
extern void PGM_INT(void);
extern void MCH_INT(void);

/**
 * local_int_disable - disable interruptions & return old mask
 */
#define local_int_disable() ({ \
	unsigned long __flags; \
	__asm__ __volatile__ ( \
		"stnsm 0(%1),0xfc" : "=m" (__flags) : "a" (&__flags) ); \
	__flags; \
	})

/**
 * local_int_restore - restore interrupt mask
 * x:	mask to restore
 */
#define local_int_restore(x) \
	__asm__ __volatile__("ssm   0(%0)" : : "a" (&x), "m" (x) : "memory")

/**
 * local_int_restore - restore interrupt mask
 * x:	mask to restore
 */
static inline int interruptable()
{
	u8 x;

	__asm__ __volatile__(
		"stosm   0(%0),0x00"
		: /* out */
		: /* in */
		 "a" (&x),
		 "m" (x)
		: /* clobber */
		 "memory"
	);

	return (x & 0x43) != 0;
}

extern void set_timer(void);

/*
 * The Supervisor-Service call table
 */
#define SVC_SCHEDULE		0
#define SVC_SCHEDULE_BLOCKED	1
#define SVC_SCHEDULE_EXIT	2
#define NR_SVC			3
extern u64 svc_table[NR_SVC];

/* Interrupt handlers */
extern void __pgm_int_handler(void);
extern void __ext_int_handler(void);
extern void __io_int_handler(void);

/* Interrupt handler stack pointer */
extern u8 *int_stack_ptr;

#endif
