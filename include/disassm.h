#ifndef __DISASSM_H
#define __DISASSM_H

enum inst_fmt {
	IF_INV = 0,
	IF_VAR,
	IF_E,
	IF_I,
	IF_RI1,
	IF_RI2,
	IF_RIE,
	IF_RIL1,
	IF_RIL2,
	IF_RIS,
	IF_RR,
	IF_RRE,
	IF_RRF1,
	IF_RRF2,
	IF_RRF3,
	IF_RRR,
	IF_RRS,
	IF_RS1,
	IF_RS2,
	IF_RSI,
	IF_RSL,
	IF_RSY1,
	IF_RSY2,
	IF_RX,
	IF_RXE,
	IF_RXF,
	IF_RXY,
	IF_S,
	IF_SI,
	IF_SIL,
	IF_SIY,
	IF_SS1,
	IF_SS2,
	IF_SS3,
	IF_SS4,
	IF_SS5,
	IF_SSE,
	IF_SSF,
	IF_DIAG,
};

struct disassm_instruction {
	union {
		char *name;	/* instruction name; inst only */

		void *ptr;	/* next level table; table only */
	} u;

	/* all */
	char fmt;		/* instruction format */

	/* table only */
	char len;		/* table len */
	char loc;		/* sub-opcode location offset in bits */
};

#define DA_INST_INV()			{ .fmt = IF_INV, }
#define DA_INST(ifmt,iname)		{ .fmt = IF_##ifmt,	.u.name = #iname, }
#define DA_INST_TBL(itbl,ilen,iloc)	{ .fmt = IF_VAR,	.u.ptr = (itbl), \
					  .len = (ilen),	.loc = (iloc), }

extern int disassm(unsigned char *bytes, char *buf, int buflen);

#endif
