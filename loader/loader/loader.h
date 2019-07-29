#ifndef __LOADER_H
#define __LOADER_H

/*
 * "Config" values
 */
#define TEMP_BASE	((unsigned char*) 0x400000) /* 4MB */
#define SYMTAB_BASE	((unsigned char*) 8192) /* 8kB */

/* 3390 with 0 key length, and 4096 data length */
#define RECORDS_PER_CYL		(15*12)
#define RECORDS_PER_TRACK	12

/* the nucleus fn ft (in EBCDIC) => HVF ELF */
#define CP_FN	"\xC8\xE5\xC6\x40\x40\x40\x40\x40"
#define CP_FT	"\xC5\xD3\xC6\x40\x40\x40\x40\x40"

/*
 * string manipulation
 */
#define memcpy(d,s,l)	__builtin_memcpy((d), (s), (l))
#define memcmp(d,s,l)	__builtin_memcmp((d), (s), (l))
#define memset(s,c,n)	__builtin_memset((s),(c),(n))

/*
 * I/O related structs, macros & variables
 */
struct ccw {
	u8 cmd;			/* Command code */
	u8 flags;		/* Flags */
	u16 count;		/* Count */
	u32 addr;		/* Data Address */
} __attribute__((packed,aligned(8)));

#define CCW_FLAG_CD		0x80	/* Chain-Data */
#define CCW_FLAG_CC		0x40	/* Chain-Command */
#define CCW_FLAG_SLI		0x20	/* Suppress-Length-Indication */
#define CCW_FLAG_SKP		0x10	/* Skip */
#define CCW_FLAG_PCI		0x08	/* Program-Controlled-Interruption */
#define CCW_FLAG_IDA		0x04	/* Indirect-Data-Address */
#define CCW_FLAG_S		0x02	/* Suspend */
#define CCW_FLAG_MIDA		0x01	/* Modified-Indirect-Data-Address */

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

#define ADDR31(x)	((u32) (u64) (x))

extern void __do_io();
extern void PGMHANDLER();

extern void load_nucleus(void);

extern struct orb ORB;

/*
 * EDF related structs & macros
 */
#define EDF_LABEL_BLOCK_NO		3
#define EDF_SUPPORTED_BLOCK_SIZE	4096

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

#endif
