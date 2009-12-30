#ifndef __EDF_H
#define __EDF_H

#include <mutex.h>
#include <device.h>

struct ADT {
	u32     ADTIDENT;       /* VOL START / LABEL IDENTIFIER */
#define __ADTIDENT 0xC3D4E2F1   /* 'CMS1' in EBCDIC */
	u8      ADTID[6];       /* VOL START / VOL IDENTIFIER */
	u8      ADTVER[2];      /* VERSION LEVEL */
	u32     ADTDBSIZ;       /* DISK BLOCK SIZE */
	u32     ADTDOP;         /* DISK ORIGIN POINTER */
	u32     ADTCYL;         /* NUM OF FORMATTED CYL ON DISK */
	u32     ADTMCYL;        /* MAX NUM FORMATTED CYL ON DISK */
	u32     ADTNUM;         /* Number of Blocks on disk */
	u32     ADTUSED;        /* Number of Blocks used */
	u32     ADTFSTSZ;       /* SIZE OF FST */
	u32     ADTNFST;        /* NUMBER OF FST'S PER BLOCK */
	u8      ADTDCRED[6];    /* DISK CREATION DATE (YYMMDDHHMMSS) */
	u8      ADTFLGL;        /* LABEL FLAG BYTE (ADTFLGL) */
#define ADTCNTRY        0x01    /* Century for disk creation date (0=19, 1=20),
	                         * corresponds to ADTDCRED. */
	u8      reserved[1];
	u32     ADTOFFST;       /* DISK OFFSET WHEN RESERVED */
	u32     ADTAMNB;        /* ALLOC MAP BLOCK WITH NEXT HOLE */
	u32     ADTAMND;        /* DISP INTO HBLK DATA OF NEXT HOLE */
	u32     ADTAMUP;        /* DISP INTO USER PART OF ALLOC MAP */
	u32     ADTOFCNT;       /* Count of SFS open files for this ADT */
	u8      ADTSFNAM[8];    /* NAME OF SHARED SEGMENT */
};

struct FST {
	u8    FSTFNAME[8];       /* filename */
	u8    FSTFTYPE[8];       /* filetype */
	u8    FSTDATEW[2];       /* DATE LAST WRITTEN - MMDD */
	u8    FSTTIMEW[2];       /* TIME LAST WRITTEN - HHMM */
	u16   FSTWRPNT;          /* WRITE POINTER - ITEM NUMBER */
	u16   FSTRDPNT;          /* READ POINTER - ITEM NUMBER */
	u8    FSTFMODE[2];       /* FILE MODE - LETTER AND NUMBER */
	u16   FSTRECCT;          /* NUMBER OF LOGICAL RECORDS */
	u16   FSTFCLPT;          /* FIRST CHAIN LINK POINTER */
	u8    FSTRECFM;          /* F*1 - RECORD FORMAT - F OR V */
#define FSTDFIX        0xC6 /* Fixed record format (EBCDIC 'F') */
#define FSTDVAR        0xE5 /* Variable record format (EBCDIC 'V') */
	u8    FSTFLAGS;          /* F*2 - FST FLAG BYTE */
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
	u32   FSTLRECL;          /* LOGICAL RECORD LENGTH */
	u16   FSTBLKCT;          /* NUMBER OF 800 BYTE BLOCKS */
	u16   FSTYEARW;          /* YEAR LAST WRITTEN */
	u32   FSTFOP;            /* ALT. FILE ORIGIN POINTER */
	u32   FSTADBC;           /* ALT. NUMBER OF DATA BLOCKS */
	u32   FSTAIC;            /* ALT. ITEM COUNT */
	u8    FSTNLVL;           /* NUMBER OF POINTER BLOCK LEVELS */
	u8    FSTPTRSZ;          /* LENGTH OF A POINTER ELEMENT */
	u8    FSTADATI[6];       /* ALT. DATE/TIME(YY MM DD HH MM SS) */
	u8    FSTREALM;          /* Real filemode */
	u8    FSTFLAG2;          /* F*3 - FST FLAG BYTE 2 FSTFLAG2 */
#define FSTPIPEU       0x10 /* Reserved for CMS PIPELINES usage */
	u8    reserved[2];
};

#define EDF_LABEL_BLOCK_NO		3

#define EDF_SUPPORTED_BLOCK_SIZE	4096

struct fs {
	struct ADT ADT;
	struct list_head files;
	mutex_t lock;
	struct device *dev;
	void *tmp_buf;
};

struct file {
	struct FST FST;
	struct list_head files;
	mutex_t lock;
	struct fs *fs;
	char *buf;
};

extern struct fs *edf_mount(struct device *dev);
extern struct file *edf_lookup(struct fs *fs, char *fn, char *ft);
extern int edf_read_rec(struct file *file, char *buf, u32 recno);

#endif
