/*
 * Copyright (c) 2007-2011 Josef 'Jeff' Sipek
 */

#include "loader.h"
#include <binfmt_elf.h>
#include <string.h>
#include <ebcdic.h>

static unsigned char seek_data[6];
static unsigned char search_data[5];

extern struct orb ORB;

static u8 *buf = (u8*) (16 * 1024);
static u32 *ptrbuf = (u32*) (20 * 1024);

static u64 pgm_new_psw_diswait[2] = {
	0x0002000180000000ULL, 0,
};

static u64 io_new_psw[2] = {
	0x0000000180000000ULL, (u64) &IOHANDLER,
};

static u64 pgm_new_psw[2] = {
	0x0000000180000000ULL, (u64) &PGMHANDLER,
};

u64 ipl_sch;
u64 con_sch;
u64 dasd_sch;

#if 0
static void read_blk(void *ptr, u32 lba)
{
	struct ccw ccw[4];

	u16 cc, hh, r;

	if (lba < 1)
		die();

	memset(ccw, 0, sizeof(ccw));

	lba--;
	cc = lba / RECORDS_PER_CYL;
	hh = (lba % RECORDS_PER_CYL) / RECORDS_PER_TRACK;
	r = (lba % RECORDS_PER_CYL) % RECORDS_PER_TRACK;
	r++;

	ORB.addr = ADDR31(ccw);

	/* SEEK */
	ccw[0].cmd = 0x07;
	ccw[0].flags = CCW_FLAG_CC | CCW_FLAG_SLI;
	ccw[0].count = 6;
	ccw[0].addr = ADDR31(seek_data);

	seek_data[0] = 0;		/* zero */
	seek_data[1] = 0;		/* zero */
	seek_data[2] = cc >> 8;		/* Cc */
	seek_data[3] = cc & 0xff;	/* cC */
	seek_data[4] = hh >> 8;		/* Hh */
	seek_data[5] = hh & 0xff;	/* hH */

	/* SEARCH */
	ccw[1].cmd = 0x31;
	ccw[1].flags = CCW_FLAG_CC | CCW_FLAG_SLI;
	ccw[1].count = 5;
	ccw[1].addr = ADDR31(search_data);

	search_data[0] = cc >> 8;
	search_data[1] = cc & 0xff;
	search_data[2] = hh >> 8;
	search_data[3] = hh & 0xff;
	search_data[4] = r;

	/* TIC */
	ccw[2].cmd = 0x08;
	ccw[2].flags = 0;
	ccw[2].count = 0;
	ccw[2].addr = ADDR31(&ccw[1]);

	/* READ DATA */
	ccw[3].cmd = 0x86;
	ccw[3].flags = 0;
	ccw[3].count = 4096;
	ccw[3].addr = ADDR31(ptr);

	/*
	 * issue IO
	 */
	__do_io(dasd_sch);
}

/*
 * read the entire nucleus into TEMP_BASE
 */
static inline void readnucleus(void)
{
	int i, found;
	u32 nfst;

	read_blk(buf, EDF_LABEL_BLOCK_NO);

	if ((ADT->IDENT != __ADTIDENT) ||
	    (ADT->DBSIZ != EDF_SUPPORTED_BLOCK_SIZE) ||
	    (ADT->OFFST != 0) ||
	    (ADT->FSTSZ != sizeof(struct FST)))
		die();

	nfst = ADT->NFST;

	read_blk(buf, ADT->DOP);

	if (FST->NLVL != 0)
	       die(); // FIXME

	for(i=0,found=0; i<nfst; i++) {
		if ((!memcmp(FST[i].FNAME, CP_FN, 8)) &&
		    (!memcmp(FST[i].FTYPE, CP_FT, 8))) {
			memcpy(&fst, &FST[i], sizeof(struct FST));
			found = 1;
			break;
		}
	}

	if (!found)
		die();

	if (fst.PTRSZ != 4 ||
	    fst.LRECL != 4096 ||
	    fst.RECFM != FSTDFIX)
		die();

	/* Don't allow more than 3MB to be read */
	if ((FST->AIC * FST->LRECL) > (3ULL * 1024 * 1024))
		die();

	/* Since we're assuming that NLVL==1, there's only 1 pointer block */
	read_blk(ptrbuf, fst.FOP);

	/* Read all the blocks pointed to by the ptr block */
	for(i=0; i<fst.AIC; i++)
		read_blk(TEMP_BASE + (4096 * i), ptrbuf[i]);
}
#endif

struct senseid_struct {
	u8 __reserved;
	u16 cu_type;
	u8 cu_model;
	u16 dev_type;
	u8 dev_model;
} __attribute__((packed));


static int dev_dasd(void)
{
	int ret;
	struct ccw ccw;
	struct senseid_struct id;

	ccw.cmd   = 0xe4;
	ccw.flags = 0;
	ccw.count = sizeof(struct senseid_struct);
	ccw.addr  = ADDR31(&id);

	ORB.param = 0x12345678,
	ORB.f     = 1,
	ORB.lpm   = 0xff,
	ORB.addr  = ADDR31(&ccw);

	ret = __do_io(dasd_sch);
	if (ret)
		die();

	if ((id.dev_type  == 0x3390) &&
	    (id.dev_model == 0x0A) &&
	    (id.cu_type   == 0x3990) &&
	    (id.cu_model  == 0xC2))
		return 0; // ECKD 3390-3 that we know how to use

	return -1; // not a DASD
}

static u64 atoi(char *s, int len)
{
	u64 i = 0;

	while(len) {
		 if ((*s >= '0') && (*s <= '9'))
			i = (i * 16) + (*s - '0');
		 else if ((*s >= 'A') && (*s <= 'F'))
			i = (i * 16) + (*s - 'A' + 10);
		 else if ((*s >= 'a') && (*s <= 'f'))
			i = (i * 16) + (*s - 'a' + 10);
		 else
			 break;
		len--;
		s++;
	}

	return i;
}

static u64 find_devnum(u64 devnum)
{
	struct schib schib;
	u64 sch;

	if (devnum > 0xffff)
		die();

	memset(&schib, 0, sizeof(struct schib));

	/*
	 * For each possible subchannel id...
	 */
	for(sch = 0x10000; sch <= 0x1ffff; sch++) {
		/*
		 * ...call store subchannel, to find out whether or not
		 * there is a device
		 */
		if (store_sch(sch, &schib))
			continue;

		if (!schib.pmcw.v)
			continue;

		if (schib.pmcw.dev_num != devnum)
			continue;

		schib.pmcw.e = 1;

		if (modify_sch(sch, &schib))
			die();

		return sch;
	}

	return ~0;
}

void wto(char *str)
{
	int ret;
	struct ccw ccw;
	char buf[160];

	strncpy(buf, str, 160);
	buf[159] = '\0';

	ascii2ebcdic((u8*)buf, 160);

	ccw.cmd   = 0x01;
	ccw.flags = 0;
	ccw.count = strlen(str);
	ccw.addr  = ADDR31(buf);

	ORB.param = 0x12345678,
	ORB.f     = 1,
	ORB.lpm   = 0xff,
	ORB.addr  = ADDR31(&ccw);

	ret = __do_io(con_sch);
	if (ret)
		die();
}

void wtor(char *str, char *inp, int buflen)
{
	int ret;
	struct ccw ccw;

	wto(str);

	/* wait for user input */
	__wait_for_attn();

	/* read user input */
	ccw.cmd   = 0x0a;
	ccw.flags = 0;
	ccw.count = buflen;
	ccw.addr  = ADDR31(inp);

	ORB.param = 0x12345678,
	ORB.f     = 1,
	ORB.lpm   = 0xff,
	ORB.addr  = ADDR31(&ccw);

	ret = __do_io(con_sch);
	if (ret)
		die();

	ebcdic2ascii((u8*)inp, buflen);
}

static void init_io(void)
{
	u64 cr6;

	/* enable all I/O interrupt classes */
	asm volatile(
		"stctg	6,6,%0\n"	/* get cr6 */
		"oi	%1,0xff\n"	/* enable all */
		"lctlg	6,6,%0\n"	/* reload cr6 */
	: /* output */
	: /* input */
	  "m" (cr6),
	  "m" (*(u64*) (((u8*)&cr6) + 4))
	);
}

void load_nucleus(void)
{
	char inp[160];

	/* Save the IPL device subchannel id */
	ipl_sch = *((u32*) 0xb8);

	memcpy((void*) 0x1d0, pgm_new_psw_diswait, 16);
	memcpy((void*) 0x1f0, io_new_psw, 16);

	init_io();

	/*
	 * try to find the console on 0009
	 */
	con_sch = find_devnum(CON_DEVNUM);
	if (con_sch > 0x1ffff)
		die();

	/*
	 * greet the user on the console
	 */
	wto("HVF installer\n\n");

	for(;;) {
		/*
		 * ask the user for target dasd
		 */
		wtor("Specify target device (1-4 hex digits):\n", inp, 160);

		dasd_sch = find_devnum(atoi(inp, 160));
		if ((dasd_sch < 0x10000) || (dasd_sch > 0x1ffff)) {
			wto("Invalid device number.\n\n");
			continue;
		}

		wto("Device found.\n\n");
		if (dev_dasd()) {
			wto("Need a DASD.\n\n");
			continue;
		}

		break;
	}

	inp[0] = 'n';
	wtor("Format volume? [y/n]\n", inp, 160);

	if ((inp[0] == 'y') || (inp[0] == 'Y')) {
		wto("Formating volume...\n");

		/*
		 * FIXME:
		 * 1) format the volume
		 * 2) setup EDF
		 */

		wto("done.\n");
	} else
		wto("Formatting skipped.\n");

	/*
	 * read through the archive and decide what to do with each file
	 */
	unload_archive();

	/*
	 * FIXME: inform the user that we're done, and load a psw with the
	 * right magic
	 */

	wto("\nInstallation complete.\n");
	wto("You can now IPL from the DASD.\n");

	for(;;);
	die();
}
