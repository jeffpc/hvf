#ifndef __LOADER_H
#define __LOADER_H

#include <errno.h>
#include <channel.h>

/*
 * "Config" values
 */
#define TEMP_BASE	((unsigned char*) 0x200000) /* 2MB */

#define CON_DEVNUM	0x0009

/* 3390 with 0 key length, and 4096 data length */
#define RECORDS_PER_CYL		(15*12)
#define RECORDS_PER_TRACK	12

/* the nucleus fn ft (in EBCDIC) => HVF ELF */
#define CP_FN	"\xC8\xE5\xC6\x40\x40\x40\x40\x40"
#define CP_FT	"\xC5\xD3\xC6\x40\x40\x40\x40\x40"

static inline int strlen(char *p)
{
	int i;

	for(i=0;*p; p++, i++)
		;

	return i;
}

/*
 * halt the cpu
 *
 * NOTE: we don't care about not clobbering registers as when this
 * code executes, the CPU will be stopped.
 */
static inline void die(void)
{
	asm volatile(
		"SR	%r1, %r1	# not used, but should be zero\n"
		"SR	%r3, %r3 	# CPU Address\n"
		"SIGP	%r1, %r3, 0x05	# Signal, order 0x05\n"
	);

	/*
	 * Just in case SIGP fails
	 */
	for(;;);
}

#define ADDR31(x)	((u32) (u64) (x))

extern void wto(char *str);
extern void wtor(char *str, char *inp, int buflen);
extern void __readwrite_blk(void *ptr, u32 lba, int rwccw);
#define read_blk(ptr, lba)	__readwrite_blk((ptr), (lba), 0x86)
#define write_blk(ptr, lba)	__readwrite_blk((ptr), (lba), 0x05)

extern int __do_io(u32 sch);
extern void __wait_for_attn();
extern void PGMHANDLER();
extern void IOHANDLER();

extern void load_nucleus(void);

extern struct orb ORB;

extern u64 ipl_sch;
extern u64 con_sch;
extern u64 dasd_sch;

extern void unload_archive(void);

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

extern void init_malloc(void *ptr);
extern void *malloc(u32 size);

extern void mount_fs();
extern int find_file(char *fn, char *ft, struct FST *fst);
extern int create_file(char *fn, char *ft, int lrecl, struct FST *fst);
extern void append_record(struct FST *fst, u8 *buf);

extern void writeback_buffers();

#endif
