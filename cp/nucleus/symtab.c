/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <symtab.h>

Elf64_Ehdr *symtab;

static Elf64_Shdr *__get_section(int i)
{
	return ((void*)symtab) + symtab->e_shoff + symtab->e_shentsize * i;
}

static void *get_section(int type, int *entsize, int *size)
{
	Elf64_Shdr *cur;
	int getstr = 0;
	int i;

	/* we want the string table associated with the symbol table */
	if (type == SHT_STRTAB) {
		getstr = 1;
		type = SHT_SYMTAB;
	}

	for(i=0; i<symtab->e_shnum; i++) {
		cur = __get_section(i);

		if (cur->sh_type == type)
			goto found;
	}

	return NULL;

found:

	if (getstr)
		cur = __get_section(cur->sh_link);

	*entsize = cur->sh_entsize;
	*size = cur->sh_size;
	return ((void*)symtab) + cur->sh_offset;
}

void symtab_find_text_range(u64 *start, u64 *end)
{
	u64 s, e;
	Elf64_Sym *cur, *tab;
	int entsize, size;
	int bind, type;

	s = -1;
	e = 0;

	tab = get_section(SHT_SYMTAB, &entsize, &size);
	if (!tab)
		return;

	cur=tab;

	while(((void*)cur)-((void*)tab)<size) {
		cur = ((void*)cur) + entsize;

		bind = cur->st_info >> 4;
		type = cur->st_info & 0xf;

		if ((bind != STB_LOCAL) && (bind != STB_GLOBAL))
			continue;

		if ((type != STT_OBJECT) && (type != STT_FUNC))
			continue;

		if (s > cur->st_value)
			s = cur->st_value;
		if (e < (cur->st_value + cur->st_size))
			e = cur->st_value + cur->st_size;
	}

	*start = s;
	*end = e;
}

char *symtab_lookup(u64 addr, char *buf, int buflen)
{
	Elf64_Sym *cur, *tab;
	char *strtab;
	int entsize, size;
	int strsize, dummy;
	int bind, type;

	tab = get_section(SHT_SYMTAB, &entsize, &size);
	if (!tab)
		goto fail;

	strtab = get_section(SHT_STRTAB, &dummy, &strsize);
	if (!strtab)
		goto fail;

	cur=tab;

	while(((void*)cur)-((void*)tab)<size) {
		cur = ((void*)cur) + entsize;

		bind = cur->st_info >> 4;
		type = cur->st_info & 0xf;

		if ((bind != STB_LOCAL) && (bind != STB_GLOBAL))
			continue;

		if ((type != STT_OBJECT) && (type != STT_FUNC))
			continue;

		if (addr < cur->st_value)
			continue;
		if (addr >= (cur->st_value + cur->st_size))
			continue;

		/* ok, found the symbol */
		if (cur->st_name > strsize)
			goto fail;

		snprintf(buf, buflen, "%s+%#llx",
			 strtab + cur->st_name, addr - cur->st_value);
		return buf;
	}

fail:
	return "???";
}
