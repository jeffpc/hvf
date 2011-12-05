/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __SYMTAB_H
#define __SYMTAB_H

#include <binfmt_elf.h>

extern Elf64_Ehdr *symtab;

extern void symtab_find_text_range(u64 *start, u64 *end);
extern char *symtab_lookup(u64 addr, char *buf, int buflen);

#endif
