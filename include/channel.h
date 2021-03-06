/*
 * Copyright (c) 2007-2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __CHANNEL_H
#define __CHANNEL_H

#include <arch.h>
#include <errno.h>

/*
 * We only care about format-1 CCWs
 */
struct ccw {
	u8 cmd;			/* Command code */
	u8 flags;		/* Flags */
	u16 count;		/* Count */
	u32 addr;		/* Data Address */
} __attribute__((packed,aligned(8)));

#define CCW0_ADDR(c)	((((u64)((c)->addr_hi)) << 16) | (((u64)((c)->addr_lo))))

struct ccw0 {
	u8 cmd;			/* Command code */
	u8 addr_hi;		/* Data Address (bits 8-15) */
	u16 addr_lo;		/* Data Address (bits 16-31) */
	u8 flags;		/* Flags */
	u8 _res0;
	u16 count;		/* Count */
} __attribute__((packed,aligned(8)));

#define CCW_CMD_INVAL		0x00
#define CCW_CMD_IPL_READ	0x02
#define CCW_CMD_NOP		0x03
#define CCW_CMD_BASIC_SENSE	0x04
#define CCW_CMD_SENSE_ID	0xe4
#define CCW_CMD_TIC		0x08

#define IS_CCW_WRITE(op)	(((op)&0x03)==0x01)
#define IS_CCW_READ(op)		(((op)&0x03)==0x02)
#define IS_CCW_CONTROL(op)	(((op)&0x03)==0x03)

#define CCW_FLAG_CD		0x80	/* Chain-Data */
#define CCW_FLAG_CC		0x40	/* Chain-Command */
#define CCW_FLAG_SLI		0x20	/* Suppress-Length-Indication */
#define CCW_FLAG_SKP		0x10	/* Skip */
#define CCW_FLAG_PCI		0x08	/* Program-Controlled-Interruption */
#define CCW_FLAG_IDA		0x04	/* Indirect-Data-Address */
#define CCW_FLAG_S		0x02	/* Suspend */
#define CCW_FLAG_MIDA		0x01	/* Modified-Indirect-Data-Address */

static inline void ccw0_to_ccw1(struct ccw *out, struct ccw0 *in)
{
	/* 0x08 is TIC; format-0 CCWs allow upper nibble to be non-zero */
	out->cmd	= ((in->cmd & 0x0f) == CCW_CMD_TIC) ?
				CCW_CMD_TIC : in->cmd;
	out->flags	= in->flags;
	out->count	= in->count;
	out->addr	= CCW0_ADDR(in);
}

/*
 * ORB
 */
struct orb {
	/* word 0 */
	u32 param;		/* Interruption Parameter */

	/* word 1 */
	u8 key:4,		/* Subchannel Key */
	   s:1,			/* Suspend */
	   c:1,			/* Streaming-Mode Control */
	   m:1,			/* Modification Control */
	   y:1;			/* Synchronization Control */
	u8 f:1,			/* Format Control */
	   p:1,			/* Prefetch Control */
	   i:1,			/* Initial-Status-Interruption Control */
	   a:1,			/* Address-Limit-Checking control */
	   u:1,			/* Suppress-Suspend-Interruption Control */
	   b:1,			/* Channel-Program Type Control */
	   h:1,			/* Format-2-IDAW Control */
	   t:1;			/* 2K-IDAW Control */
	u8 lpm;			/* Logical-Path Mask */
	u8 l:1,			/* Incorrect-Length-Suppression Mode */
	   d:1,			/* Modified-CCW-Indirect-Data-Addressing Control */
	   __zero1:5,
	   x:1;			/* ORB-Extension Control */

	/* word 2 */
	u32 addr;		/* Channel-Program Address */

	/* word 3 */
	u8 css_prio;		/* Channel-Subsystem Priority */
	u8 __reserved1;
	u8 cu_prio;		/* Control-Unit Priority */
	u8 __reserved2;

	/* word 4 - 7 */
	u32 __reserved3;
	u32 __reserved4;
	u32 __reserved5;
	u32 __reserved6;
} __attribute__((packed,aligned(4)));

enum SCSW_FC {
	FC_CLEAR	= 0x1,
	FC_HALT		= 0x2,
	FC_START	= 0x4,
};

enum SCSW_AC {
	AC_RESUME       = 0x40,
	AC_START        = 0x20,
	AC_HALT         = 0x10,
	AC_CLEAR        = 0x08,
	AC_SCH_ACT      = 0x04,
	AC_DEV_ACT      = 0x02,
	AC_SUSP         = 0x01,
};

enum SCSW_SC {
	SC_STATUS	= 0x01,
	SC_SECONDARY	= 0x02,
	SC_PRIMARY	= 0x04,
	SC_INTERMED	= 0x08,
	SC_ALERT	= 0x10,
};

struct scsw {
	/* word 0 */
	u16 key:4,		/* Subchannel key */
	    s:1,		/* Suspend control */
	    l:1,		/* ESW format */
	    cc:2,		/* Deferred condition code */
	    f:1,		/* Format */
	    p:1,		/* Prefetch */
	    i:1,		/* Initial-status interruption control */
	    a:1,		/* Address-limit-checking control */
	    u:1,		/* Supress-suspended interruption */
	    z:1,		/* Zero condition code */
	    e:1,		/* Extended control */
	    n:1;		/* Path no operational */
	u16 __zero:1,
	    fc:3,		/* Function control */
	    ac:7,		/* Activity control */
	    sc:5;		/* Status control */

	/* word 1 */
	u32 addr;		/* CCW Address */

	/* word 2 */
	u8 dev_status;		/* Device status */
	u8 sch_status;		/* Subchannel status */
	u16 count;		/* Count */
} __attribute__((packed));

/* needed by IRB */
struct irb_ext_status {
	/* TODO: not implemented */
	u32 w0, w1, w2, w3, w4;
} __attribute__((packed));

/* needed by IRB */
struct irb_ext_control {
	/* TODO: not implemented */
	u32 w0, w1, w2, w3, w4, w5, w6, w7;
} __attribute__((packed));

/* needed by IRB */
struct irb_ext_measurement {
	/* TODO: not implemented */
	u32 w0, w1, w2, w3, w4, w5, w6, w7;
} __attribute__((packed));

struct irb {
	struct scsw scsw;			/* Subchannel-Status */
	struct irb_ext_status ext_status;	/* Extended-Status */
	struct irb_ext_control ext_control;	/* Extended-Control */
	struct irb_ext_measurement ext_measure;	/* Extended-Measurement */
} __attribute__((packed,aligned(4)));

/* Path Management Control Word */
struct pmcw {
	/* word 0 */
	u32 interrupt_param;	/* Interruption Parameter */

	/* word 1*/
	u8 __zero1:2,
	   isc:3,		/* I/O-Interruption-Subclass Code */
	   __zero2:3;
	u8 e:1,			/* Enabled */
	   lm:2,		/* Limit Mode */
	   mm:2,		/* Measurement-Mode Enable */
	   d:1,			/* Multipath Mode */
	   t:1,			/* Timing Facility */
	   v:1;			/* Device Number Valid */
	u16 dev_num;		/* Device Number */

	/* word 2 */
	u8 lpm;			/* Logical-Path Mask */
	u8 pnom;		/* Path-Not-Operational Mask */
	u8 lpum;		/* Last-Path-Used Mask */
	u8 pim;			/* Path-Installed Mask */

	/* word 3 */
	u16 mbi;		/* Measurement-Block Index */
	u8 pom;			/* Path-Operational Mask */
	u8 pam;			/* Path-Available Mask */

	/* word 4 & 5 */
	u8 chpid[8];		/* Channel-Path Identifiers */

	/* word 6 */
	u16 __zero3;
	u16 __zero4:13,
	    f:1,		/* Measurement Block Format Control */
	    x:1,		/* Extended Measurement Word Mode Enable */
	    s:1;		/* Concurrent Sense */
};

/* needed by schib */
struct schib_measurement_block {
	/* TODO: not implemented */
	u32 w0, w1;
};

struct schib {
	struct pmcw pmcw;		/* Path Management Control Word */
	struct scsw scsw;		/* Subchannel Status Word */
	union {
		struct schib_measurement_block measure_block;
	};
	u32 model_dep_area;
} __attribute__((packed,aligned(4)));

struct senseid_struct {
	u8 __reserved;
	u16 cu_type;
	u8 cu_model;
	u16 dev_type;
	u8 dev_model;
} __attribute__((packed));

enum {
	SC_STATUS_ATTN = 0x80, /* Attention */
	SC_STATUS_MOD  = 0x40, /* Status Modifier */
	SC_STATUS_CUE  = 0x20, /* Control-Unit End */
	SC_STATUS_BUSY = 0x10, /* Busy */
	SC_STATUS_CE   = 0x08, /* Channel End */
	SC_STATUS_DE   = 0x04, /* Device End */
	SC_STATUS_UC   = 0x02, /* Unit-check */
	SC_STATUS_UE   = 0x01, /* Unit-exception */
};

enum {
	SC_PROG_CI  = 0x80, /* Program-Controlled Interruption */
	SC_INC_LEN  = 0x40, /* Incorrect Length */
	SC_PROG_CHK = 0x20, /* Program Check */
	SC_PROT_CHK = 0x10, /* Protection Check */
	SC_CHD_CHK  = 0x08, /* Channel-Data Check */
	SC_CHC_CHK  = 0x04, /* Channel-Control Check */
	SC_IC_CHK   = 0x02, /* Interface-Control Check */
	SC_CHAIN_CHK= 0x01, /* Chaining Check */
};

enum {
	SENSE_CMDREJ = 0x00,
};

static inline int store_sch(u32 sch, struct schib *schib)
{
	int cc;

	asm volatile(
		"lr	%%r1,%2\n"
		"stsch	%1\n"
		"ipm	%0\n"
		"srl	%0,28\n"
		: /* output */
		  "=d" (cc),
		  "=Q" (*schib)
		: /* input */
		  "d" (sch)
		: /* clobbered */
		  "cc", "r1", "memory"
	);

	if (cc == 3)
		return -EINVAL;
	return 0;
}

static inline int modify_sch(u32 sch, struct schib *schib)
{
	int cc;

	asm volatile(
		"lr	%%r1,%1\n"
		"msch	0(%2)\n"
		"ipm	%0\n"
		"srl	%0,28\n"
		: /* output */
		  "=d" (cc)
		: /* input */
		  "d" (sch),
		  "a" (schib)
		: /* clobbered */
		  "cc", "r1"
	);

	if (cc == 1 || cc == 2)
		return -EBUSY;
	if (cc == 3)
		return -EINVAL;
	return 0;
}

static inline int start_sch(u32 sch, struct orb *orb)
{
	int cc;

	asm volatile(
		"	lr	%%r1,%1\n"
		"	ssch	0(%2)\n"
		"	ipm	%0\n"
		"	srl	%0,28\n"
		: /* output */
		  "=d" (cc)
		: /* input */
		  "d" (sch),
		  "a" (orb)
		: /* clobbered */
		  "cc", "r1"
	);

	if (cc == 1 || cc == 2)
		return -EBUSY;
	if (cc == 3)
		return -EINVAL;

	return 0;
}

static inline int test_sch(u32 sch, struct irb *irb)
{
	int cc;

	asm volatile(
		"	lr	%%r1,%1\n"
		"	tsch	0(%2)\n"
		"	ipm	%0\n"
		"	srl	%0,28\n"
	: /* output */
	  "=d" (cc)
	: /* input */
	  "d" (sch),
	  "a" (irb)
	: /* clobbered */
	  "cc", "r1"
	);

	if (cc == 3)
		return -EINVAL;

	return 0;
}

struct crw {
	u16 _pad0:1,
	    s:1,
	    r:1,
	    c:1,
	    rsc:4,
	    a:1,
	    _pad1:1,
	    erc:6;
	u16 id;
} __attribute__((packed));

/**
 * store_crw - stores the Channel Report Word
 * @crw:	pointer to CRW
 * @return:	0 on success, 1 if zeros were stored
 */
static inline int store_crw(struct crw *crw)
{
	int cc;

	asm volatile(
		"	stcrw	%1\n"
		"	ipm	%0\n"
		"	srl	%0,28\n"
	: /* output */
	  "=d" (cc),
	  "=m" (*crw)
	: /* input */
	: /* clobbered */
	  "cc", "r1"
	);

	return cc;
}

static inline void enable_io_int_classes(uint8_t classes)
{
	uint64_t cr6;

	cr6 = get_cr(6);
	cr6 |= ((uint64_t) classes) << BIT64_SHIFT(39);
	set_cr(6, cr6);
}

static inline void disable_io_int_classes(uint8_t classes)
{
	uint64_t cr6;

	cr6 = get_cr(6);
	cr6 &= ~(((uint64_t) classes) << BIT64_SHIFT(39));
	set_cr(6, cr6);
}

#endif
