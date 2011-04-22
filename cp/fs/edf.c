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

static LOCK_CLASS(edf_fs);
static LOCK_CLASS(edf_file);

struct fs *edf_mount(struct device *dev, int nosched)
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

	fs->nosched = nosched;

	/* First, read & verify the label */
	ret = bdev_read_block(dev, tmp, EDF_LABEL_BLOCK_NO, nosched);
	if (ret)
		goto out_free;

	mutex_init(&fs->lock, &edf_fs);
	INIT_LIST_HEAD(&fs->files);
	fs->dev = dev;
	fs->tmp_buf = tmp;

	memcpy(&fs->ADT, tmp, sizeof(struct ADT));

	ret = -EINVAL;
	if ((fs->ADT.ADTIDENT != __ADTIDENT) ||
	    (fs->ADT.ADTDBSIZ != EDF_SUPPORTED_BLOCK_SIZE) ||
	    (fs->ADT.ADTOFFST != 0) ||
	    (fs->ADT.ADTFSTSZ != sizeof(struct FST)))
		goto out_free;

	return fs;

out_free:
	free(fs);
	free_pages(tmp, 0);
	return ERR_PTR(ret);
}

extern struct console *oper_con;
struct file *edf_lookup(struct fs *fs, char *fn, char *ft)
{
	char __fn[8];
	char __ft[8];
	struct page *page;
	struct file *file;
	struct file *tmpf;
	struct FST *fst;
	long ret;
	int found;
	int i;

	file = malloc(sizeof(struct file), ZONE_NORMAL);
	if (!file)
		return ERR_PTR(-ENOMEM);

	file->fs = fs;
	file->buf = NULL;

	memcpy(__fn, fn, 8);
	memcpy(__ft, ft, 8);
	ascii2ebcdic((u8 *) __fn, 8);
	ascii2ebcdic((u8 *) __ft, 8);

	mutex_lock(&fs->lock);

	/* first, check the cache */
	list_for_each_entry(tmpf, &fs->files, files) {
		if (!memcmp((char*) tmpf->FST.FSTFNAME, __fn, 8) &&
		    !memcmp((char*) tmpf->FST.FSTFTYPE, __ft, 8)) {
			mutex_unlock(&fs->lock);
			free(file);
			return tmpf;
		}
	}

	page = alloc_pages(0, ZONE_NORMAL);
	if (!page) {
		ret = -ENOMEM;
		goto out_unlock;
	}
	file->buf = page_to_addr(page);

	/* oh well, must do it the hard way ... read from disk */
	ret = bdev_read_block(fs->dev, fs->tmp_buf, fs->ADT.ADTDOP,
			      fs->nosched);
	if (ret)
		goto out_unlock;

	fst = fs->tmp_buf;

	for(i=0,found=0; i<fs->ADT.ADTNFST; i++) {
		if ((!memcmp(fst[i].FSTFNAME, __fn, 8)) &&
		    (!memcmp(fst[i].FSTFTYPE, __ft, 8))) {
			memcpy(&file->FST, &fst[i], sizeof(struct FST));
			found = 1;
			break;
		}
	}

	if (!found) {
		ret = -ENOENT;
		goto out_unlock;
	}

	mutex_init(&file->lock, &edf_file);
	list_add_tail(&file->files, &fs->files);

	mutex_unlock(&fs->lock);

	return file;

out_unlock:
	if (file && file->buf)
		free_pages(file->buf, 0);
	mutex_unlock(&fs->lock);
	free(file);
	return ERR_PTR(ret);
}

int edf_read_rec(struct file *file, char *buf, u32 recno)
{
	struct fs *fs = file->fs;
	u32 fop, lrecl;
	int ret;

	if (file->FST.FSTNLVL != 0 ||
	    file->FST.FSTPTRSZ != 4 ||
	    file->FST.FSTLRECL > fs->ADT.ADTDBSIZ ||
	    file->FST.FSTRECFM != FSTDFIX)
		return -EINVAL;

	mutex_lock(&file->lock);

	fop = file->FST.FSTFOP;
	lrecl = file->FST.FSTLRECL;

	ret = bdev_read_block(fs->dev, file->buf, fop, fs->nosched);
	if (ret)
		goto out;

	memcpy(buf, file->buf + (recno * lrecl), lrecl);

out:
	mutex_unlock(&file->lock);

	return ret;
}

void edf_file_free(struct file *file)
{
	struct fs *fs = file->fs;

	mutex_lock(&fs->lock);
	list_del(&file->files);
	mutex_unlock(&fs->lock);

	free(file);
}
