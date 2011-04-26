/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __BINFMT_ELF_H
#define __BINFMT_ELF_H

#define EI_MAG0		0
#define EI_MAG1		1
#define EI_MAG2		2
#define EI_MAG3		3
#define EI_CLASS	4
#define EI_DATA		5
#define EI_VERSION	6
#define EI_OSABI	7
#define EI_ABIVERSION	8
#define EI_PAD		9
#define EI_NIDENT	16

#define ELFMAG0		0x7f
#define ELFMAG1		'E'
#define ELFMAG2		'L'
#define ELFMAG3		'F'

#define ELFCLASSNONE	0
#define ELFCLASS32	1
#define ELFCLASS64	2

#define ELFDATANONE	0
#define ELFDATA2LSB	1
#define ELFDATA2MSB	2

#define EV_CURRENT	1

#define ELFOSABI_NONE	0

#define ET_NONE		0
#define ET_REL		1
#define ET_EXEC		2
#define ET_DYN		3
#define ET_CORE		4
#define ET_LOOS		0xfe00
#define ET_HIOS		0xfeff
#define ET_LOPROC	0xff00
#define ET_HIPROC	0xffff

#define EM_S390		22

#define SHT_NULL		0
#define SHT_PROGBITS		1
#define SHT_SYMTAB		2
#define SHT_STRTAB		3
#define SHT_RELA		4
#define SHT_HASH		5
#define SHT_DYNAMIC		6
#define SHT_NOTE		7
#define SHT_NOBITS		8
#define SHT_REL			9
#define SHT_SHLIB		10
#define SHT_DYNSYM		11
#define SHT_INIT_ARRAY		14
#define SHT_FINI_ARRAY		15
#define SHT_PREINI_ARRAY	16
#define SHT_GROUP		17
#define SHT_SYMTAB_SHNDX	18
#define SHT_LOOS		0x60000000
#define SHT_HIOS		0x6fffffff
#define SHT_LOPROC		0x70000000
#define SHT_HIPROC		0x7fffffff
#define SHT_LOUSER		0x80000000
#define SHT_HIUSER		0xffffffff

#define PT_NULL		0
#define PT_LOAD		1
#define PT_DYNAMIC	2
#define PT_INTERP	3
#define PT_NOTE		4
#define PT_SHLIB	5
#define PT_PHDR		6
#define PT_TLS		7
#define PT_LOOS		0x60000000
#define PT_HIOS		0x6fffffff
#define PT_LOPROC	0x70000000
#define PT_HIPROC	0x7fffffff

#define PF_X		0x1
#define PF_W		0x2
#define PF_R		0x4
#define PF_MASKOS	0x0ff00000
#define PF_MAKSPROC	0xf0000000

typedef u32 Elf32_Addr;
typedef u32 Elf32_Off;
typedef u16 Elf32_Half;
typedef u32 Elf32_Word;
typedef s32 Elf32_Sword;

typedef u64 Elf64_Addr;
typedef u64 Elf64_Off;
typedef u16 Elf64_Half;
typedef u32 Elf64_Word;
typedef s32 Elf64_Sword;
typedef u64 Elf64_Xword;
typedef s64 Elf64_Sxword;

/*
 * ELF file header
 */
typedef struct {
	unsigned char   e_ident[EI_NIDENT];	/* ELF identification */
	Elf32_Half      e_type;			/* Object file type */
	Elf32_Half      e_machine;		/* Machine type */
	Elf32_Word      e_version;		/* Object file version */
	Elf32_Addr      e_entry;		/* Entry point address */
	Elf32_Off       e_phoff;		/* Program header offset */
	Elf32_Off       e_shoff;		/* Section header offset */
	Elf32_Word      e_flags;		/* Processor-specific flags */
	Elf32_Half      e_ehsize;		/* ELF header size */
	Elf32_Half      e_phentsize;		/* Size of program header entry */
	Elf32_Half      e_phnum;		/* Number of program header entries */
	Elf32_Half      e_shentsize;		/* Size of section header entries */
	Elf32_Half      e_shnum;		/* Number of section header entries */
	Elf32_Half      e_shstrndx;		/* Section name string table index */
} Elf32_Ehdr;

typedef struct {
	unsigned char   e_ident[EI_NIDENT];	/* ELF identification */
	Elf64_Half      e_type;			/* Object file type */
	Elf64_Half      e_machine;		/* Machine type */
	Elf64_Word      e_version;		/* Object file version */
	Elf64_Addr      e_entry;		/* Entry point address */
	Elf64_Off       e_phoff;		/* Program header offset */
	Elf64_Off       e_shoff;		/* Section header offset */
	Elf64_Word      e_flags;		/* Processor-specific flags */
	Elf64_Half      e_ehsize;		/* ELF header size */
	Elf64_Half      e_phentsize;		/* Size of program header entry */
	Elf64_Half      e_phnum;		/* Number of program header entries */
	Elf64_Half      e_shentsize;		/* Size of section header entries */
	Elf64_Half      e_shnum;		/* Number of section header entries */
	Elf64_Half      e_shstrndx;		/* Section name string table index */
} Elf64_Ehdr;

/* combined header for easy pointer casting */
typedef union {
	Elf32_Ehdr s390;
	Elf64_Ehdr z;
} Elf_Ehdr;

/*
 * ELF section header
 */
typedef struct {
	Elf32_Word	sh_name;		/* Section name */
	Elf32_Word	sh_type;		/* Section type */
	Elf32_Word	sh_flags;		/* Section attributes */
	Elf32_Addr	sh_addr;		/* Virtual address in memory */
	Elf32_Off	sh_offset;		/* Offset in file */
	Elf32_Word	sh_size;		/* Size of section */
	Elf32_Word	sh_link;		/* Link to other section */
	Elf32_Word	sh_info;		/* Misc information */
	Elf32_Word	sh_addralign;		/* Address alignment boundary */
	Elf32_Word	sh_entsize;		/* Size of entries, if section has table */
} Elf32_Shdr;

typedef struct {
	Elf64_Word	sh_name;		/* Section name */
	Elf64_Word	sh_type;		/* Section type */
	Elf64_Xword	sh_flags;		/* Section attributes */
	Elf64_Addr	sh_addr;		/* Virtual address in memory */
	Elf64_Off	sh_offset;		/* Offset in file */
	Elf64_Xword	sh_size;		/* Size of section */
	Elf64_Word	sh_link;		/* Link to other section */
	Elf64_Word	sh_info;		/* Misc information */
	Elf64_Xword	sh_addralign;		/* Address alignment boundary */
	Elf64_Xword	sh_entsize;		/* Size of entries, if section has table */
} Elf64_Shdr;

/* combined header for easy pointer casting */
typedef union {
	Elf32_Shdr s390;
	Elf64_Shdr z;
} Elf_Shdr;

/*
 * ELF program header
 */
typedef struct {
	Elf32_Word	p_type;			/* Segment type */
	Elf32_Off	p_offset;		/* Segment file offset */
	Elf32_Addr	p_vaddr;		/* Segment virt. addr */
	Elf32_Addr	p_paddr;		/* <undefined> */
	Elf32_Word	p_filesz;		/* Segment size in file */
	Elf32_Word	p_memsz;		/* Segment size in mem */
	Elf32_Word	p_flags;		/* Segment flags */
	Elf32_Word	p_align;		/* Segment alignment */
} Elf32_Phdr;

typedef struct {
	Elf64_Word	p_type;			/* Segment type */
	Elf64_Word	p_flags;		/* Segment file offset */
	Elf64_Off	p_offset;		/* Segment virt. addr */
	Elf64_Addr	p_vaddr;		/* <undefined> */
	Elf64_Addr	p_paddr;		/* Segment size in file */
	Elf64_Xword	p_filesz;		/* Segment size in mem */
	Elf64_Xword	p_memsz;		/* Segment flags */
	Elf64_Xword	p_align;		/* Segment alignment */
} Elf64_Phdr;

/* combined header for easy pointer casting */
typedef union {
	Elf32_Phdr s390;
	Elf64_Phdr z;
} Elf_Phdr;

#endif
