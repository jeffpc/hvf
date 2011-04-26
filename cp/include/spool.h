/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __SPOOL_H
#define __SPOOL_H

#include <list.h>
#include <mutex.h>

struct spool_rec {
	u16 len;
	u8 data[0];
};

#define SPOOL_DATA_SIZE		(PAGE_SIZE - sizeof(struct list_head))
struct spool_page {
	struct list_head list;
	u8 data[SPOOL_DATA_SIZE];
};

struct spool_file {
	mutex_t lock;
	struct list_head list;
	u64 recs;
	u64 pages;
	u16 frecoff;	/* start of the first record,
			   offset into first page's data */
	u16 lrecoff;	/* end of the last record,
			   offset into last page's data */
};

extern struct spool_file *alloc_spool();
extern void free_spool(struct spool_file *f);
extern int spool_append_rec(struct spool_file *f, u8 *buf, u16 len);

#endif
