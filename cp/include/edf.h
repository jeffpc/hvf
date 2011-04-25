/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __EDF_H
#define __EDF_H

#include <mutex.h>
#include <device.h>

struct ADT {
	u32     IDENT;       /* VOL START / LABEL IDENTIFIER */
#define __ADTIDENT 0xC3D4E2F1   /* 'CMS1' in EBCDIC */
	u8      ID[6];       /* VOL START / VOL IDENTIFIER */
	u8      VER[2];      /* VERSION LEVEL */
	u32     DBSIZ;       /* DISK BLOCK SIZE */
	u32     DOP;         /* DISK ORIGIN POINTER */
	u32     CYL;         /* NUM OF FORMATTED CYL ON DISK */
	u32     MCYL;        /* MAX NUM FORMATTED CYL ON DISK */
	u32     NUM;         /* Number of Blocks on disk */
	u32     USED;        /* Number of Blocks used */
	u32     FSTSZ;       /* SIZE OF FST */
	u32     NFST;        /* NUMBER OF FST'S PER BLOCK */
	u8      DCRED[6];    /* DISK CREATION DATE (YYMMDDHHMMSS) */
	u8      FLGL;        /* LABEL FLAG BYTE (ADTFLGL) */
#define ADTCNTRY        0x01    /* Century for disk creation date (0=19, 1=20),
	                         * corresponds to ADTDCRED. */
	u8      reserved[1];
	u32     OFFST;       /* DISK OFFSET WHEN RESERVED */
	u32     AMNB;        /* ALLOC MAP BLOCK WITH NEXT HOLE */
	u32     AMND;        /* DISP INTO HBLK DATA OF NEXT HOLE */
	u32     AMUP;        /* DISP INTO USER PART OF ALLOC MAP */
	u32     OFCNT;       /* Count of SFS open files for this ADT */
	u8      SFNAM[8];    /* NAME OF SHARED SEGMENT */
};

struct FST {
	u8    FNAME[8];       /* filename */
	u8    FTYPE[8];       /* filetype */
	u8    DATEW[2];       /* DATE LAST WRITTEN - MMDD */
	u8    TIMEW[2];       /* TIME LAST WRITTEN - HHMM */
	u16   WRPNT;          /* WRITE POINTER - ITEM NUMBER */
	u16   RDPNT;          /* READ POINTER - ITEM NUMBER */
	u8    FMODE[2];       /* FILE MODE - LETTER AND NUMBER */
	u16   RECCT;          /* NUMBER OF LOGICAL RECORDS */
	u16   FCLPT;          /* FIRST CHAIN LINK POINTER */
	u8    RECFM;          /* F*1 - RECORD FORMAT - F OR V */
#define FSTDFIX        0xC6 /* Fixed record format (EBCDIC 'F') */
#define FSTDVAR        0xE5 /* Variable record format (EBCDIC 'V') */
	u8    FLAGS;          /* F*2 - FST FLAG BYTE */
#define FSTRWDSK       0x80 /* READ/WRITE DISK */
#define FSTRODSK       0x00 /* READ/ONLY DISK */
#define FSTDSFS        0x10 /* Shared File FST */
#define FSTXRDSK       0x40 /* EXTENSION OF R/O DISK */
#define FSTXWDSK       0xC0 /* EXTENSION OF R/W DISK */
#define FSTEPL         0x20 /* EXTENDED PLIST */
#define FSTDIA         0x40 /* ITEM AVAILABLE */
#define FSTDRA         0x01 /* PREVIOUS RECORD NULL */
#define FSTCNTRY       0x08 /* Century for date last written (0=19, 1=20),\\
			       corresponds to FSTYEARW, FSTADATI. */
#define FSTACTRD       0x04 /* ACTIVE FOR READING */
#define FSTACTWR       0x02 /* ACTIVE FOR WRITING */
#define FSTACTPT       0x01 /* ACTIVE FROM A POINT */
#define FSTFILEA       0x07 /* THE FILE IS ACTIVE */
	u32   LRECL;          /* LOGICAL RECORD LENGTH */
	u16   BLKCT;          /* NUMBER OF 800 BYTE BLOCKS */
	u16   YEARW;          /* YEAR LAST WRITTEN */
	u32   FOP;            /* ALT. FILE ORIGIN POINTER */
	u32   ADBC;           /* ALT. NUMBER OF DATA BLOCKS */
	u32   AIC;            /* ALT. ITEM COUNT */
	u8    NLVL;           /* NUMBER OF POINTER BLOCK LEVELS */
	u8    PTRSZ;          /* LENGTH OF A POINTER ELEMENT */
	u8    ADATI[6];       /* ALT. DATE/TIME(YY MM DD HH MM SS) */
	u8    REALM;          /* Real filemode */
	u8    FLAG2;          /* F*3 - FST FLAG BYTE 2 FSTFLAG2 */
#define FSTPIPEU       0x10 /* Reserved for CMS PIPELINES usage */
	u8    reserved[2];
};

#define EDF_LABEL_BLOCK_NO		3

#define EDF_SUPPORTED_BLOCK_SIZE	4096

#define DIRECTOR_FN	((u8*) "\x00\x00\x00\x01\x00\x00\x00\x00")
#define DIRECTOR_FT	((u8*) "\xc4\xc9\xd9\xc5\xc3\xe3\xd6\xd9")
#define ALLOCMAP_FN	((u8*) "\x00\x00\x00\x02\x00\x00\x00\x00")
#define ALLOCMAP_FT	((u8*) "\xc1\xd3\xd3\xd6\xc3\xd4\xc1\xd7")

struct file;

struct fs {
	struct ADT ADT;
	struct list_head files;
	mutex_t lock;
	struct device *dev;
	struct file *dir;
};

struct file {
	struct FST FST;
	struct list_head files; /* fs->files */
	struct list_head bcache;
	mutex_t lock;
	struct fs *fs;
};

extern struct fs *edf_mount(struct device *dev);
extern struct file *edf_lookup(struct fs *fs, char *fn, char *ft);
extern int edf_read_rec(struct file *file, char *buf, u32 recno);

#endif
