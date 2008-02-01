#ifndef __CHANNEL_H
#define __CHANNEL_H

/*
 * We only care about format-1 CCWs
 */
struct ccw {
	u8 cmd;			/* command code */
	u8 cd:1,		/* Chain-Data */
	   cc:1,		/* Chain-Command */
	   sli:1,		/* Suppress-Length-Indication */
	   skp:1,		/* Skip */
	   pci:1,		/* Program-Controlled-Interruption */
	   ida:1,		/* Indirect-Data-Address */
	   s:1,			/* Suspend */
	   mida:1;		/* Modified-Indirect-Data-Address */
	u16 count;		/* Count */
	u32 addr;		/* Data Address */
} __attribute__((packed,aligned(8)));

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
	   __zero1:1,
	   h:1,			/* Format-2-IDAW Control */
	   t:1;			/* 2K-IDAW Control */ 
	u8 lpm;			/* Logical-Path Mask */
	u8 l:1,			/* Incorrect-Length-Suppression Mode */
	   d:1,			/* Modified-CCW-Indirect-Data-Addressing Control */
	   __zero2:5,
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

struct subch_status {
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
	    ac:8,		/* Activity control */
	    sc:4;		/* Status control */

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
	struct subch_status status;		/* Subchannel-Status */
	struct irb_ext_status ext_status;	/* Extended-Status */
	struct irb_ext_control ext_control;	/* Extended-Control */
	struct irb_ext_measurement ext_measure;	/* Extended-Measurement */
} __attribute__((packed,aligned(4)));

/* needed by schib */
struct schib_pmcw {
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
	struct schib_pmcw path_ctl;
	struct subch_status status;		/* Subchannel-Status */
	union {
		struct schib_measurement_block measure_block;
	};
	u32 model_dep_area;
} __attribute__((packed,aligned(4)));

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
	  "r1"
	);

	if (cc == 3)
		return -EINVAL;

	return 0;
}

extern void scan_devices();

#endif
