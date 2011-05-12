/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __BCACHE_H
#define __BCACHE_H

#include <edf.h>

extern int bcache_add(struct file *file, int level, u32 blk, u32 lba);
extern void *bcache_read(struct file *file, int level, u32 blk);

#endif
