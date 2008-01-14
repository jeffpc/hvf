#ifndef __DAT_H
#define __DAT_H

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

/* page-table entries */
struct dat_pte {
	u64 pfra:52,		/* page-frame real address */
	    __zero0:1,
	    i:1,		/* page-invalid bit */
	    p:1,		/* page-protection bit */
	    __zero1:1,
	    __reserved:8;
} __attribute__((packed));

#endif
