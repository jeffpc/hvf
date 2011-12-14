/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <list.h>
#include <mutex.h>
#include <buddy.h>
#include <slab.h>
#include <edf.h>
#include <bdev.h>
#include <bcache.h>

struct bcache_entry {
	struct list_head bcache;

	int level; /* 0 = data */
	u32 blk;

	u32 lba;
	void *data;
};

static struct bcache_entry *__bcache_add(struct file *file, int level, u32 blk,
					 u32 lba)
{
	struct bcache_entry *ent;

	ent = malloc(sizeof(struct bcache_entry), ZONE_NORMAL);
	if (!ent)
		return ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(&ent->bcache);
	ent->level = level;
	ent->blk   = blk;
	ent->lba   = lba;
	ent->data  = NULL;

	list_add_tail(&ent->bcache, &file->bcache);

	return ent;
}

int bcache_add(struct file *file, int level, u32 blk, u32 lba)
{
	struct bcache_entry *ent;

	ent = __bcache_add(file, level, blk, lba);

	return IS_ERR(ent) ? PTR_ERR(ent) : 0;
}

void *bcache_read(struct file *file, int level, u32 blk)
{
	struct bcache_entry *cur;
	struct page *page;
	void *buf;
	u32 *ptrs;
	u32 nblk, off;
	int ret;

	BUG_ON(level > file->FST.NLVL);

	list_for_each_entry(cur, &file->bcache, bcache) {
		if ((cur->level == level) && (cur->blk == blk))
			goto found;
	}

	if (level == file->FST.NLVL) {
		/* we can get the lba for this from the FST */
		cur = __bcache_add(file, level, 0, file->FST.FOP);
		if (IS_ERR(cur))
			return cur;

		goto found;
	} else {
		/* we need to read the pointer block one level up */
		off = blk % (file->fs->ADT.DBSIZ / 4);
		nblk = blk / (file->fs->ADT.DBSIZ / 4);

		/*
		 * WARNING: recursive call below!
		 *
		 * Thankfully, there is a bound on the number of levels - 2
		 * pointer blocks worth & 1 data block level.
		 */
		ptrs = bcache_read(file, level+1, nblk);
		if (IS_ERR(ptrs))
			return ptrs;

		/* add the pointer to the cache */

		FIXME("since we read the whole block, we should probably "
		      "add all the pointers to the cache");
		cur = __bcache_add(file, level, blk, ptrs[off]);
		if (IS_ERR(cur))
			return cur;

		goto found;
	}
	return ERR_PTR(-EFAULT);

found:
	if (cur->data)
		return cur->data;

	page = alloc_pages(0, ZONE_NORMAL);
	if (!page)
		return ERR_PTR(-ENOMEM);

	buf = page_to_addr(page);

	ret = bdev_read_block(file->fs->dev, buf, cur->lba);
	if (ret) {
		free_pages(buf, 0);
		return ERR_PTR(ret);
	}

	cur->data = buf;
	return buf;
}
