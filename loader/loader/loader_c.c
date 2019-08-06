/*
 * Copyright (c) 2007-2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "loader.h"
#include <channel.h>
#include <binfmt_elf.h>

static unsigned char seek_data[6];
static unsigned char search_data[5];
static struct ccw ccw[4];

struct orb ORB = {
	.param		= 0x12345678,
	.f		= 1,
	.lpm		= 0xff,
	.addr		= 0xffffffff,
};

static u8 *buf = (u8*) (16 * 1024);
static u32 *ptrbuf = (u32*) (20 * 1024);

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

/*
 * determine amount of storage
 */
static u64 sense_memsize(void)
{
	volatile u64 *pgm_psw = (void *) 0x1d0;
	u64 size;
	int cc;

#define SKIP_SIZE	(1024*1024ULL)

	/* set new PGM psw */
	pgm_psw[0] = 0x0000000180000000ULL;
	pgm_psw[1] = (u64) &PGMHANDLER;

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
	memset((void *) pgm_psw, 0, 16);

	return size;
}

static void enable_io(void)
{
	enable_io_int_classes(0xff);
}

static void read_blk(void *ptr, u32 lba)
{
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
	__do_io();
}

/*
 * read the entire nucleus into TEMP_BASE
 */
static inline void readnucleus(void)
{
	struct ADT *ADT = (struct ADT*) buf;
	struct FST *FST = (struct FST*) buf;
	struct FST fst;
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

void load_nucleus(void)
{
	/*
	 * These are all stored in registers
	 */
	register u32 iplsch;
	register int i, j;
	register Elf64_Ehdr *nucleus_elf;
	register Elf64_Shdr *section, *link_section, *tmp_section;
	register Elf64_Phdr *segment;
	register Elf64_Ehdr *elfcpy;
	register unsigned char *symtab;
	register void (*start_sym)(u64, u32, void*);

	/* Save the IPL device subchannel id */
	iplsch = *((u32*) 0xb8);

	/*
	 * Enable I/O
	 */
	enable_io();

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

	/* Iterate through each program header, and copy all PT_LOAD
	 * segments to the final destinations.
	 */

	for(i=0; i<nucleus_elf->e_phnum; i++) {
		segment = (Elf64_Phdr*) (TEMP_BASE +
					 nucleus_elf->e_phoff +
					 nucleus_elf->e_phentsize * i);

		if (segment->p_type != PT_LOAD)
			continue;

		if (segment->p_filesz)
			memcpy((void*) segment->p_vaddr,
			       TEMP_BASE + segment->p_offset,
			       segment->p_filesz);

		if (segment->p_filesz != segment->p_memsz)
			memset((void*) (segment->p_vaddr + segment->p_filesz),
			       0, segment->p_memsz - segment->p_filesz);
	}

	/*
	 * Copy over the ELF header & tweak it.
	 */
	symtab = SYMTAB_BASE;
	elfcpy = (Elf64_Ehdr*) symtab;
	memcpy(elfcpy, nucleus_elf, nucleus_elf->e_ehsize);
	symtab += nucleus_elf->e_ehsize;

	/* the program headers start right after */
	elfcpy->e_phoff = symtab - SYMTAB_BASE;

	/* copy over all the program headers */
	memcpy(symtab, TEMP_BASE + nucleus_elf->e_phoff,
	       nucleus_elf->e_phentsize * nucleus_elf->e_phnum);
	symtab += nucleus_elf->e_phentsize * nucleus_elf->e_phnum;

	/* the section headers start right after */
	elfcpy->e_shoff = symtab - SYMTAB_BASE;

	/*
	 * Iterate through each section to find the .symtab & .strtab (as
	 * well as the null section), and copy the the headers over
	 */
	elfcpy->e_shnum = 0;

	for(i=0; i<nucleus_elf->e_shnum; i++) {
		section = (Elf64_Shdr*) (TEMP_BASE +
					 nucleus_elf->e_shoff +
					 nucleus_elf->e_shentsize * i);

		switch (section->sh_type) {
			case SHT_STRTAB:
				if (i == nucleus_elf->e_shstrndx)
					elfcpy->e_shstrndx = elfcpy->e_shnum;

			case SHT_NULL:
			case SHT_SYMTAB:
				memcpy(symtab, section, nucleus_elf->e_shentsize);
				elfcpy->e_shnum++;
				symtab += nucleus_elf->e_shentsize;
				break;
			default:
				/* Ignoring */
				break;
		}
	}

	for(i=0; i<elfcpy->e_shnum; i++) {
		section = (Elf64_Shdr*) (SYMTAB_BASE +
					 elfcpy->e_shoff +
					 elfcpy->e_shentsize * i);

		if (!section->sh_link)
			continue;

		tmp_section = (Elf64_Shdr*) (TEMP_BASE +
					 nucleus_elf->e_shoff +
					 nucleus_elf->e_shentsize *
					 section->sh_link);

		for(j=0; j<elfcpy->e_shnum; j++) {
			link_section = (Elf64_Shdr*) (SYMTAB_BASE +
						      elfcpy->e_shoff +
						      elfcpy->e_shentsize * j);

			if (memcmp(tmp_section, link_section, elfcpy->e_shentsize))
				continue;

			section->sh_link = j;
			break;
		}
	}

	/*
	 * Now that we know what sections and where, let's copy those over
	 */
	for (i=1; i<elfcpy->e_shnum; i++) {
		section = (Elf64_Shdr*) (SYMTAB_BASE +
					 elfcpy->e_shoff +
					 elfcpy->e_shentsize * i);

		memcpy(symtab, TEMP_BASE + section->sh_offset,
		       section->sh_size);
		section->sh_offset = symtab - SYMTAB_BASE;
		symtab += section->sh_size;
	}

	/*
	 * Now, jump to the nucleus entry point
	 */
	start_sym = (void*) nucleus_elf->e_entry;
	start_sym(sense_memsize(), iplsch, SYMTAB_BASE);
}
