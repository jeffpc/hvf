/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __ARCH_H
#define __ARCH_H

/*
 * PSW and PSW handling
 */
struct psw {
	u8 _zero0:1,
	   r:1,			/* PER Mask (R)			*/
	   _zero1:3,
	   t:1,			/* DAT Mode (T)			*/
	   io:1,		/* I/O Mask (IO)		*/
	   ex:1;		/* External Mask (EX)		*/

	u8 key:4,		/* Key				*/
	   fmt:1,		/* 1 on 390, 0 on z		*/
	   m:1,			/* Machine-Check Mask (M)	*/
	   w:1,			/* Wait State (W)		*/
	   p:1;			/* Problem State (P)		*/

	u8 as:2,		/* Address-Space Control (AS)	*/
	   cc:2,		/* Condition Code (CC)		*/
	   prog_mask:4;		/* Program Mask			*/

	u8 _zero2:7,
	   ea:1;		/* Extended Addressing (EA)	*/

	u32 ba:1,		/* Basic Addressing (BA)	*/
	    ptr31:31;

	u64 ptr;
} __attribute__((packed,aligned(8)));

static inline void lpswe(struct psw *psw)
{
	asm volatile(
		"	lpswe	%0\n"
	: /* output */
	: /* input */
	  "m" (*psw)
	);
}

/*
 * Control Register handling
 */
#define set_cr(cr, val)						\
	do {							\
		u64 lval = (val);				\
		asm volatile(					\
			"lctlg	%1,%1,%0\n"			\
		: /* output */					\
		: /* input */					\
		  "m" (lval),					\
		  "i" (cr)					\
		);						\
	} while(0)

#define get_cr(cr)						\
	({							\
		u64 reg;					\
								\
		asm volatile(					\
			"stctg	%1,%1,%0\n"			\
		: /* output */					\
		  "=m" (reg)					\
		: /* input */					\
		  "i" (cr)					\
		);						\
		reg;						\
	 })

/*
 * Machine Check Interrupt related structs & locations
 */
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

#define MCH_INT_OLD_PSW ((void*) 0x160)
#define MCH_INT_NEW_PSW ((void*) 0x1e0)
#define MCH_INT_CODE	((struct mch_int_code*) 0xe8)

#endif
