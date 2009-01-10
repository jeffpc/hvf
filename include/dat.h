#ifndef __DAT_H
#define __DAT_H

struct address_space {
	struct dat_rte *region_table;
	struct dat_ste *segment_table;
};

/* region/segment-table destination */
struct dat_td {
	u64 origin:52,		/* region/segment table origin */
	    __reserved0:2,
	    g:1,		/* subspace-group control */
	    p:1,		/* private-space control */
	    s:1,		/* storage-alteration-event ctl */
	    x:1,		/* space-switch-event control */
	    r:1,		/* real-space control */
	    __reserved1:1,
	    dt:2,		/* designation-type control */
	    tl:2;		/* table length */
} __attribute__((packed));

#define DAT_TD_DT_RFT		3	/* region-first */
#define DAT_TD_DT_RST		2	/* region-second */
#define DAT_TD_DT_RTT		1	/* region-third */
#define DAT_TD_DT_ST		0	/* segment */

/* region-table entries (first, second, and third) */
struct dat_rte {
	u64 origin:52,		/* next-level-table origin */
	    __reserved0:4,
	    tf:2,		/* table offset (next lower tbl) */
	    i:1,		/* region invalid */
	    __reserved1:1,
	    tt:2,		/* table-type bits (current tbl) */
	    tl:2;		/* table length (next lower tbl) */
} __attribute__((packed));

/*
 * Constants for struct rte
 */
#define DAT_RTE_TT_RFT		3	/* region-first */
#define DAT_RTE_TT_RST		2	/* region-second */
#define DAT_RTE_TT_RTT		1	/* region-third */

#define ADDR_TO_RTE_ORIGIN(a)	((a) >> 12)
#define RTE_ORIGIN_TO_ADDR(o)	((void*)(((u64)(o)) << 12))

/* segment-table entries */
struct dat_ste {
	u64 origin:53,		/* page-table origin */
	    __reserved0:1,
	    p:1,		/* page-protection bit */
	    __reserved1:3,
	    i:1,		/* segment-invalid bit */
	    c:1,		/* common-segment bit */
	    tt:2,		/* table-type bits */
	    __reserved2:2;
} __attribute__((packed));

#define DAT_STE_TT_ST		0	/* segment-table */

#define ADDR_TO_STE_ORIGIN(a)	((a) >> 11)
#define STE_ORIGIN_TO_ADDR(o)	((void*)(((u64)(o)) << 11))

/* page-table entries */
struct dat_pte {
	u64 pfra:52,		/* page-frame real address */
	    __zero0:1,
	    i:1,		/* page-invalid bit */
	    p:1,		/* page-protection bit */
	    __zero1:1,
	    __reserved:8;
} __attribute__((packed));

#define DAT_RX(addr)	(((u64)(addr)) >> 31)
#define DAT_SX(addr)	((((u64)(addr)) >> 20) & 0x7ff)
#define DAT_PX(addr)	((((u64)(addr)) >> 12) & 0xff)
#define DAT_BX(addr)	(((u64)(addr))& 0xfff)

extern int dat_insert_page(struct address_space *as, u64 phy, u64 virt);
extern void setup_dat();
extern void load_as(struct address_space *as);

extern int virt2phy(struct address_space *as, u64 virt, u64 *phy);

static inline void store_pasce(u64 *pasce)
{
	asm volatile(
		"	stctg	1,1,0(%0)\n"
	: /* output */
	: /* input */
	  "a" (pasce)
	);
}

static inline void load_pasce(u64 pasce)
{
	if (!pasce)
		return;

	asm volatile(
		"	lctlg	1,1,%0\n"
	: /* output */
	: /* input */
	  "m" (pasce)
	);
}

#endif
