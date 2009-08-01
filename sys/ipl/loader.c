/*
 * Copyright (c) 2007 Josef 'Jeff' Sipek
 */

#ifndef BYTES_TO_READ
#error missing BYTES_TO_READ
#endif

#if BYTES_TO_READ > 0x300000
#error The nucleus size is limited to 3MB
#endif

#ifndef BLOCK_SIZE
#error missing BLOCK_SIZE
#endif

#include <loader.h>
#include <binfmt_elf.h>

static unsigned char seek_ccw[8] __attribute__ ((aligned (8))) = {
	/*
	 * CCW1; seek to the right place
	 */

	0x3F,
	/*   bits  value   name                        desc             */
	/*    0-7      2   Cmd Code                    ?, modifiers     */

	0x00,0x00,0x00,
	/*   bits  value   name                        desc             */
	/*   8-31   addr   Data Address                dest of the read */

	0x20,
	/*   bits  value   name                        desc             */
	/*     32      0   Chain-Data (CD)             don't chain      */
	/*     33      0   Chain-Command (CC)          don't chain      */
	/*     34      1   Sup.-Len.-Inditcation (SLI) suppress         */
	/*     35      0   Skip (SKP)                  issue read       */
	/*     36      0   Prog.-Contr.-Inter. (PCI)   don't interrupt  */
	/*     37      0   Indir.-Data-Addr. (IDA)     real addr        */
	/*     38      0   Suspend (S)                 don't suspend    */
	/*     39      0   Modified I.D.A. (MIDA)      real addr        */

	0x00,
	/*   bits  value   name                        desc             */
	/*  40-47      0   <ignored>                                    */

	0x00,0x01,
	/*   bits  value   name                        desc             */
	/*  48-63    len   number of bytes to read                      */
};

static unsigned char read_ccw[8] __attribute__ ((aligned (8))) = {
	/*
	 * CCW2; read the entire nucleus ELF
	 */

	0x02,
	/*   bits  value   name                        desc             */
	/*    0-7      2   Cmd Code                    read, no modifiers*/

	0xff, 0xff, 0xff,
	/*   bits  value   name                        desc             */
	/*   8-31   addr   Data Address                dest of the read */

	0x20,
	/*   bits  value   name                        desc             */
	/*     32      0   Chain-Data (CD)             don't chain      */
	/*     33      0   Chain-Command (CC)          don't chain      */
	/*     34      1   Sup.-Len.-Inditcation (SLI) suppress         */
	/*     35      0   Skip (SKP)                  issue read       */
	/*     36      0   Prog.-Contr.-Inter. (PCI)   don't interrupt  */
	/*     37      0   Indir.-Data-Addr. (IDA)     real addr        */
	/*     38      0   Suspend (S)                 don't suspend    */
	/*     39      0   Modified I.D.A. (MIDA)      real addr        */

	0x00,
	/*   bits  value   name                        desc             */
	/*  40-47      0   <ignored>                                    */

	0xff, 0xff,
	/*   bits  value   name                        desc             */
	/*  48-63    len   number of bytes to read                      */
};

unsigned char ORB[32] __attribute__ ((aligned (16))) = {
	/* Word 0 */
	0x12,0x34,0x56,0x78,
	/*   bits  value   name                        desc             */
	/*   0-31  magic   Interrupt Parameter                          */

	/* Word 1 */
	0x00,
	/*   bits  value   name                        desc             */
	/*    0-3      0   Subchannel Key                               */
	/*      4      0   Suspend Control                              */
	/*      5      0   Streaming-Mode Control                       */
	/*      6      0   Modification Control                         */
	/*      7      0   Synchronization Control                      */

	0x00,
	/*   bits  value   name                        desc             */
	/*      8      0   Format Control              format-0 CCWs    */
	/*      9      0   Prefetch Control                             */
	/*     10      0   Initial-Status-Interruption Control          */
	/*     11      0   Address-Limit-Checking Control               */
	/*     12      0   Suppress-Suspended-Interruption Control      */
	/*     13      0   <zero>                                       */
	/*     14      0   Format-2-IDAW Control                        */
	/*     15      0   2K-IDAW Control                              */

	0xff,
	/*   bits  value   name                        desc             */
	/*  16-23   0xff   Logical-Path Mask           All paths        */

	0x00,
	/*   bits  value   name                        desc             */
	/*     24      0   Incorrect-Length-Suppression Mode            */
	/*     25      0   Modified-CCW-Indirect-Data-Addressing Control*/
	/*  26-30      0   <zero>                                       */
	/*     31      0   ORB-Extension Control                        */

	/* Word 2 */
	0xff,0xff,0xff,0xff,
	/*   bits  value   name                        desc             */
	/*   0-31   addr   Channel-Program Address                      */

	/* Word 3 */
	0x00,
	/*   bits  value   name                        desc             */
	/*    0-7      0   Channel-Subsystem Priority                   */

	0x00,
	/*   bits  value   name                        desc             */
	/*   8-15      0   <zero/reserved>                              */

	0x00,
	/*   bits  value   name                        desc             */
	/*  15-23      0   Control-Unit Priority                        */

	0x00,
	/*   bits  value   name                        desc             */
	/*  24-31      0   <zero/reserved>                              */

	/* Word 4 */
	0x00,0x00,0x00,0x00,
	/*   bits  value   name                        desc             */
	/*   0-31      0   <zero/reserved>                              */

	/* Word 5 */
	0x00,0x00,0x00,0x00,
	/*   bits  value   name                        desc             */
	/*   0-31      0   <zero/reserved>                              */

	/* Word 6 */
	0x00,0x00,0x00,0x00,
	/*   bits  value   name                        desc             */
	/*   0-31      0   <zero/reserved>                              */

	/* Word 7 */
	0x00,0x00,0x00,0x00,
	/*   bits  value   name                        desc             */
	/*   0-31      0   <zero/reserved>                              */
};

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

static u64 pgm_new_psw[2] = {
	0x0000000180000000ULL, (u64) &PGMHANDLER,
};

/*
 * determine amount of storage
 */
static u64 sense_memsize(void)
{
	u64 size;
	int cc;

#define SKIP_SIZE	(1024*1024ULL)

	/* set new PGM psw */
	memcpy((void*)0x1d0, pgm_new_psw, 16);

	for(size = 0; size < ((u64)~SKIP_SIZE)-1; size += SKIP_SIZE) {
		asm volatile(
			"lg	%%r1,%1\n"
			"tprot	0(%%r1),0\n"
			"ipm	%0\n"
			"srl    %0,28\n"
		: /* output */
		  "=d" (cc)
		: /* input */
		  "m" (size)
		: /* clobber */
		  "cc", "r1"
		);

		/*
		 * we cheat here a little...if we try to tprot a location
		 * that isn't part of the configuration, a program exception
		 * fires off, but our handler sets the CC to 3, and resumes
		 * execution
		 */
		if (cc == 3)
			break;
	}

	/* invalidate new PGM psw */
	memset((void*)0x1d0, 0, 16);

	return size;
}

/*
 * read the entire nucleus into into TEMP_BASE
 */
static inline void readnucleus(void)
{
	register unsigned long base;

	/*
	 * First, seek past the tape mark
	 */

	/* set the CCW address in the ORB */
	*((u32 *) &ORB[8]) = (u32) (u64) seek_ccw;

#ifdef TAPE_SEEK
	/*
	 * Seek to the next tape mark
	 */
	__do_io();
#endif

	/*
	 * Second, read in BLOCK_SIZE chunks of nucleus
	 */

	/* set the CCW address in the ORB */
	*((u32 *) &ORB[8]) = (u32) (u64) read_ccw;

	read_ccw[6] = ((unsigned char) (BLOCK_SIZE >> 8) & 0xff);
	read_ccw[7] = ((unsigned char) (BLOCK_SIZE & 0xff));

	base = (unsigned long) TEMP_BASE;
	for( ;
	    (base - (unsigned long)TEMP_BASE) < BYTES_TO_READ;
	    base += BLOCK_SIZE) {
		read_ccw[1] = ((unsigned char) (base >> 16));
		read_ccw[2] = ((unsigned char) (base >> 8) & 0xff);
		read_ccw[3] = ((unsigned char) (base & 0xff));
		__do_io();
	}
}

void load_nucleus(void)
{
	/*
	 * These are all stored in registers
	 */
	register int i;
	register Elf64_Ehdr *nucleus_elf;
	register Elf64_Shdr *section;
	register void (*start_sym)(u64);

	/*
	 * Read entire ELF to temporary location
	 */
	readnucleus();

	nucleus_elf = (Elf64_Ehdr*) TEMP_BASE;

	/*
	 * Check that this looks like a valid ELF
	 */
	if (nucleus_elf->e_ident[0] != '\x7f' ||
	    nucleus_elf->e_ident[1] != 'E' ||
	    nucleus_elf->e_ident[2] != 'L' ||
	    nucleus_elf->e_ident[3] != 'F' ||
	    nucleus_elf->e_ident[EI_CLASS] != ELFCLASS64 ||
	    nucleus_elf->e_ident[EI_DATA] != ELFDATA2MSB ||
	    nucleus_elf->e_ident[EI_VERSION] != EV_CURRENT ||
	    nucleus_elf->e_type != ET_EXEC ||
	    nucleus_elf->e_machine != 0x16 || // FIXME: find the name for the #define
	    nucleus_elf->e_version != EV_CURRENT)
		die();

	/*
	 * Iterate through each section, and copy it to the final
	 * destination as necessary
	 */
	for (i=0; i<nucleus_elf->e_shnum; i++) {
		section = (Elf64_Shdr*) (TEMP_BASE +
					 nucleus_elf->e_shoff +
					 nucleus_elf->e_shentsize * i);

		switch (section->sh_type) {
			case SHT_PROGBITS:
				if (!section->sh_addr)
					break;

				/*
				 * just copy the data from TEMP_BASE to
				 * where it wants to be
				 */
				memcpy((void*) section->sh_addr,
					TEMP_BASE + section->sh_offset,
					section->sh_size);
				break;
			case SHT_NOBITS:
				/*
				 * No action needed as there's no data to
				 * copy, and we assume that the ELF sections
				 * don't overlap
				 */
				memset((void*) section->sh_addr,
				       0, section->sh_size);
				break;
			case SHT_SYMTAB:
			case SHT_STRTAB:
				/* TODO: relocate */
				break;
			default:
				/* Ignoring */
				break;
		}
	}

	/*
	 * Now, jump to the nucleus entry point
	 */
	start_sym = (void*) nucleus_elf->e_entry;
	start_sym(sense_memsize());
}
