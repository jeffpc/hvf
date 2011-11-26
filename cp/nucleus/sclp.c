/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <ebcdic.h>
#include <vsprintf.h>
#include <stdarg.h>
#include <sclp.h>

struct sccb_header {
	u16	len;
	u8	flags;
	u8	__reserved[2];
	u8	type;
	u8	reason;
	u8	response;
} __attribute__((packed));

struct sccb_evd_header {
	u16	len;
	u8	type;
	u8	flags;
	u16	__reserved;
} __attribute__((packed));

struct sccb_mcd_header {
	u16	len;
	u16	type;
	u32	tag;
	u32	code;
} __attribute__((packed));

struct sccb_obj_header {
	u16	len;
	u16	type;
} __attribute__((packed));

struct sccb_mto_header {
	u8	ltflags[2];
	u8	present[4];
} __attribute__((packed));

struct sccb {
	struct sccb_header hdr;
	struct sccb_evd_header evd;
	struct sccb_mcd_header mcd;
	struct sccb_obj_header obj;
	struct sccb_mto_header mto;
	char txt[256];
} __attribute__((aligned(8)));

void sclp_msg(const char *fmt, ...)
{
	struct sccb sccb;
	va_list args;
	int ret;

	memset(&sccb, 0, sizeof(sccb));

	va_start(args, fmt);
	ret = vsnprintf(sccb.txt, sizeof(sccb.txt), fmt, args);
	va_end(args);

	if (!ret)
		return;

	ascii2ebcdic((u8*) sccb.txt, ret);

	/* Note: sccb.mto is ok all zeroed out */
#define SCCB_OBJ_TYPE_MESSAGE	0x0004
	sccb.obj.len      = ret + sizeof(struct sccb_mto_header) +
				sizeof(struct sccb_obj_header);
	sccb.obj.type	  = SCCB_OBJ_TYPE_MESSAGE;

	sccb.mcd.len      = sccb.obj.len + sizeof(struct sccb_mcd_header);
	sccb.mcd.type     = 0x0001;
	sccb.mcd.tag      = 0xd4c4c240;
	sccb.mcd.code     = 0x00000001;

#define SCCB_EVD_TYPE_PRIOR	0x09
	sccb.evd.len      = sccb.mcd.len + sizeof(struct sccb_evd_header);
	sccb.evd.type     = SCCB_EVD_TYPE_PRIOR;

#define SCCB_FLAGS_SYNC		0x80
#define SCCB_WRITE_EVENT_DATA	0x00760005
	sccb.hdr.len      = sccb.evd.len + sizeof(struct sccb_header);
	sccb.hdr.flags    = SCCB_FLAGS_SYNC;

	/*
	 * SERVC r1,r2
	 *  r1 = command
	 *  r2 = real addr of control block
	 */
	int cc;
	int cmd = SCCB_WRITE_EVENT_DATA;
	asm volatile(
		     "       .insn rre,0xb2200000,%1,%2\n"  /* servc %1,%2 */
		     "       ipm     %0\n"
		     "       srl     %0,28"
		     : "=&d" (cc) : "d" (cmd), "a" (&sccb)
		     : "cc", "memory");
}
