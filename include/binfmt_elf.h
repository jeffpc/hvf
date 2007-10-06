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

#define ELFCLASS32	1
#define ELFCLASS64	2

#define ELFDATA2LSB	1
#define ELFDATA2MSB	2

#define EV_CURRENT	1

#define ET_NONE		0
#define ET_REL		1
#define ET_EXEC		2
#define ET_DYN		3
#define ET_CORE		4
#define ET_LOOS		0xfe00
#define ET_HIOS		0xfeff
#define ET_LOPROC	0xff00
#define ET_HIPROC	0xffff

#define SHT_NULL	0
#define SHT_PROGBITS	1
#define SHT_SYMTAB	2
#define SHT_STRTAB	3
#define SHT_RELA	4
#define SHT_HASH	5
#define SHT_DYNAMIC	6
#define SHT_NOTE	7
#define SHT_NOBITS	8
#define SHT_REL		9
#define SHT_SHLIB	10
#define SHT_DYNSYM	11
#define SHT_LOOS	0x60000000
#define SHT_HIOS	0x6fffffff
#define SHT_LOPROC	0x70000000
#define SHT_HIPROC	0x7fffffff

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

/*
 * ELF section header
 */
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

#endif
