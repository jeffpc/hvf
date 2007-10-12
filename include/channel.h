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

/* part of IRB */
struct irb_subch_status {
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
	struct irb_subch_status status;		/* Subchannel-Status */
	struct irb_ext_status ext_status;	/* Extended-Status */
	struct irb_ext_control ext_control;	/* Extended-Control */
	struct irb_ext_measurement ext_measure;	/* Extended-Measurement */
} __attribute__((packed,aligned(4)));

#endif
