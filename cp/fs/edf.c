/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <mutex.h>
#include <buddy.h>
#include <slab.h>
#include <device.h>
#include <bdev.h>
#include <ebcdic.h>
#include <edf.h>
#include <bcache.h>

static LOCK_CLASS(edf_fs);
static LOCK_CLASS(edf_dir);
static LOCK_CLASS(edf_file);

/* assumes that fs->lock is held */
static struct file *__alloc_file(struct fs *fs, struct FST *fst)
{
	struct file *file;

	file = malloc(sizeof(struct file), ZONE_NORMAL);
	if (!file)
		return ERR_PTR(-ENOMEM);

	memset(file, 0, sizeof(struct file));

	if (fst)
		memcpy(&file->FST, fst, sizeof(struct FST));

	INIT_LIST_HEAD(&file->files);
	INIT_LIST_HEAD(&file->bcache);
	mutex_init(&file->lock, &edf_file);

	file->fs = fs;

	list_add_tail(&file->files, &fs->files);

	return file;
}

/* assumes that fs->lock is held */
static void __free_file(struct file *file)
{
	list_del(&file->files);
	free(file);
}

static int __init_dir(struct fs *fs, void *tmp)
{
	struct FST *fst = tmp;
	struct file *file;
	int ret;

	ret = bdev_read_block(fs->dev, tmp, fs->ADT.DOP);
	if (ret)
		return ret;

	if (memcmp(fst[0].FNAME, DIRECTOR_FN, 8) ||
	    memcmp(fst[0].FTYPE, DIRECTOR_FT, 8) ||
	    (fst[0].RECFM != FSTDFIX) ||
	    memcmp(fst[1].FNAME, ALLOCMAP_FN, 8) ||
	    memcmp(fst[1].FTYPE, ALLOCMAP_FT, 8) ||
	    (fst[1].RECFM != FSTDFIX))
		return -ECORRUPT;

	file = __alloc_file(fs, &fst[0]);
	if (IS_ERR(file))
		return PTR_ERR(file);

	/* need to override the default lock class */
	mutex_init(&file->lock, &edf_dir);

	ret = bcache_add(file, 0, 0, fs->ADT.DOP);
	if (ret) {
		__free_file(file);
		return ret;
	}

	fs->dir = file;

	return 0;
}

struct fs *edf_mount(struct device *dev)
{
	struct page *page;
	void *tmp;
	struct fs *fs;
	long ret;

	page = alloc_pages(0, ZONE_NORMAL);
	if (!page)
		return ERR_PTR(-ENOMEM);
	tmp = page_to_addr(page);

	ret = -ENOMEM;
	fs = malloc(sizeof(struct fs), ZONE_NORMAL);
	if (!fs)
		goto out_free;

	/* First, read & verify the label */
	ret = bdev_read_block(dev, tmp, EDF_LABEL_BLOCK_NO);
	if (ret)
		goto out_free;

	mutex_init(&fs->lock, &edf_fs);
	INIT_LIST_HEAD(&fs->files);
	fs->dev = dev;

	memcpy(&fs->ADT, tmp, sizeof(struct ADT));

	ret = -EINVAL;
	if ((fs->ADT.IDENT != __ADTIDENT) ||
	    (fs->ADT.DBSIZ != EDF_SUPPORTED_BLOCK_SIZE) ||
	    (fs->ADT.OFFST != 0) ||
	    (fs->ADT.FSTSZ != sizeof(struct FST)))
		goto out_free;

	ret = __init_dir(fs, tmp);
	if (ret)
		goto out_free;

	/* FIXME: init the ALLOCMAP */

	free_pages(tmp, 0);
	return fs;

out_free:
	free(fs);
	free_pages(tmp, 0);
	return ERR_PTR(ret);
}

struct file *edf_lookup(struct fs *fs, char *fn, char *ft)
{
	char __fn[8];
	char __ft[8];
	struct file *file;
	struct FST *fst;
	u32 blk;
	int ret;
	int i;

	memcpy(__fn, fn, 8);
	memcpy(__ft, ft, 8);
	ascii2ebcdic((u8 *) __fn, 8);
	ascii2ebcdic((u8 *) __ft, 8);

	mutex_lock(&fs->lock);

	/* first, check the cache */
	list_for_each_entry(file, &fs->files, files) {
		if (!memcmp((char*) file->FST.FNAME, __fn, 8) &&
		    !memcmp((char*) file->FST.FTYPE, __ft, 8)) {
			mutex_unlock(&fs->lock);
			return file;
		}
	}

	/* the slow path */
	file = __alloc_file(fs, NULL);
	if (IS_ERR(file)) {
		ret = PTR_ERR(file);
		goto out;
	}

	for(blk=0; blk<fs->dir->FST.ADBC; blk++) {
		fst = bcache_read(fs->dir, 0, blk);
		if (IS_ERR(fst)) {
			ret = PTR_ERR(fst);
			goto out_free;
		}

		for(i=0; i<fs->ADT.NFST; i++) {
			if ((!memcmp(fst[i].FNAME, __fn, 8)) &&
			    (!memcmp(fst[i].FTYPE, __ft, 8))) {
				memcpy(&file->FST, &fst[i], sizeof(struct FST));
				mutex_unlock(&fs->lock);
				return file;
			}
		}
	}

	ret = -ENOENT;

out_free:
	__free_file(file);

out:
	mutex_unlock(&fs->lock);
	return ERR_PTR(ret);
}

int edf_read_rec(struct file *file, char *buf, u32 recno)
{
	struct fs *fs = file->fs;
	u32 blk, off;
	char *dbuf;
	int ret = -EINVAL;

	mutex_lock(&file->lock);

	if (file->FST.NLVL != 0 ||
	    file->FST.PTRSZ != 4 ||
	    file->FST.LRECL > fs->ADT.DBSIZ ||
	    file->FST.RECFM != FSTDFIX)
		goto out;

	blk = (recno * file->FST.LRECL) / fs->ADT.DBSIZ;
	off = (recno * file->FST.LRECL) % fs->ADT.DBSIZ;

	dbuf = bcache_read(file, 0, blk);
	if (IS_ERR(dbuf)) {
		ret = PTR_ERR(dbuf);
		goto out;
	}

	BUG_ON((off + file->FST.LRECL) > fs->ADT.DBSIZ);

	memcpy(buf, dbuf + off, file->FST.LRECL);

	ret = 0;

out:
	mutex_unlock(&file->lock);

	return ret;
}

void edf_file_free(struct file *file)
{
	struct fs *fs = file->fs;

	mutex_lock(&fs->lock);
	__free_file(file);
	mutex_unlock(&fs->lock);
}
