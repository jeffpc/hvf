/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <spool.h>
#include <buddy.h>
#include <slab.h>

static LOCK_CLASS(spool_file_lc);

struct spool_file *alloc_spool()
{
	struct spool_file *f;

	f = malloc(sizeof(struct spool_file), ZONE_NORMAL);
	if (!f)
		return ERR_PTR(-ENOMEM);

	mutex_init(&f->lock, &spool_file_lc);
	INIT_LIST_HEAD(&f->list);
	f->recs = 0;
	f->pages = 0;
	f->frecoff = 0;
	f->lrecoff = 0;

	return f;
}

void free_spool(struct spool_file *f)
{
	free(f);
}

u64 spool_nrecs(struct spool_file *f)
{
	u64 recs;

	mutex_lock(&f->lock);

	recs = f->recs;

	mutex_unlock(&f->lock);

	return recs;
}

/*
 * Returns 0 on success
 *
 * Note: the output may get truncated
 */
int spool_grab_rec(struct spool_file *f, u8 *buf, u16 *len)
{
	struct spool_page *spage;
	struct spool_rec *rec;
	int ret = -ENOENT;
	u16 reclen; /* length of record */
	u32 copied, processed, rlen, offset, left;

	if (!*len || !buf || !f)
		return -EINVAL;

	BUG_ON(sizeof(struct spool_rec) != 2);
	BUG_ON(SPOOL_DATA_SIZE % 2);

	mutex_lock(&f->lock);

	if (!f->recs)
		goto out;

	BUG_ON(f->lrecoff % 2);

	copied = 0;
	processed = 0;

	rec = NULL;

	/* figure out the record length */
	spage = list_first_entry(&f->list, struct spool_page, list);
	rec = (struct spool_rec*) (spage->data + f->frecoff);
	reclen = rec->len;

	rlen = min(*len, reclen) + sizeof(struct spool_rec);
	copied = 2;
	processed = 2;
	offset = f->frecoff+2;
	left = SPOOL_DATA_SIZE - f->frecoff - 2;

	while(reclen+2 != processed) {
		if (!left) {
			/* free page */
			list_del(&spage->list);
			free_pages(spage, 0);
			f->pages--;

			/* grab the next page */
			spage = list_first_entry(&f->list, struct spool_page, list);
			offset = 0;
			left = SPOOL_DATA_SIZE;
		}

		if (rlen != copied) {
			/* memcpy into the user buffer */
			u32 tmp;

			tmp = min(left, rlen-copied);

			memcpy(buf+copied-2, &spage->data[offset], tmp);
			processed += tmp;
			copied += tmp;
			left -= tmp;
			offset += tmp;
		} else {
			/* we already filled the user buffer, but the record
			 * is longer, so we need to take it all out
			 */
			u32 tmp;

			tmp = min(left, reclen-processed+2);

			processed += tmp;
			left -= tmp;
			offset += tmp;
		}
	}

	/* align to a multiple of 2 bytes */
	if (offset % 2)
		offset++;

	if (offset == SPOOL_DATA_SIZE) {
		/* free page */
		spage = list_first_entry(&f->list, struct spool_page, list);

		list_del(&spage->list);
		free_pages(spage, 0);
		f->pages--;

		offset = 0;
	}

	f->recs--;
	f->frecoff = offset;

	*len = copied-2;
	ret = 0;

out:
	mutex_unlock(&f->lock);

	return ret;
}

int spool_append_rec(struct spool_file *f, u8 *buf, u16 len)
{
	struct list_head new_pages;
	struct spool_page *spage, *tmp;
	struct spool_rec *rec;
	struct page *page;
	int npages;
	int loff;
	int copied;
	u32 left;
	u32 rlen;
	int ret;

	if (!f || !buf || !len)
		return -EINVAL;

	INIT_LIST_HEAD(&new_pages);
	npages = 0;

	BUG_ON(sizeof(struct spool_rec) != 2);
	BUG_ON(SPOOL_DATA_SIZE % 2);

	mutex_lock(&f->lock);

	BUG_ON(f->lrecoff % 2);

	left = SPOOL_DATA_SIZE - f->lrecoff;
	rlen = len + sizeof(struct spool_rec);
	copied = 0;
	loff = 0;

	/* try to fill up the last page */
	if (f->pages && (left >= 2)) {
		spage = list_last_entry(&f->list, struct spool_page, list);
		rec = (struct spool_rec*) (spage->data + f->lrecoff);

		rec->len = len;
		copied = 2;

		if (left-2) {
			memcpy(rec->data, buf, min_t(u32, len, left-2));
			copied += min_t(u32, len, left-2);
		}

		loff = f->lrecoff + copied;
	}

	BUG_ON(rlen < copied);

	/* we need to allocate space */
	while (rlen != copied) {
		page = alloc_pages(0, ZONE_NORMAL);
		if (!page) {
			ret = -ENOMEM;
			goto err;
		}

		spage = page_to_addr(page);

		INIT_LIST_HEAD(&spage->list);
		list_add_tail(&spage->list, &new_pages);
		npages++;

		rec = (struct spool_rec*) spage->data;
		left = SPOOL_DATA_SIZE;

		if (!copied) {
			/* nothing was copied */
			rec->len = len;
			memcpy(rec->data, buf, min_t(u32, len, left-2));
			loff = 2 + min_t(u32, len, left-2);
		} else {
			/* the length and maybe some data were copied */
			memcpy(spage->data, buf-copied+2, min(rlen-copied, left));
			loff = min(rlen-copied, left);
		}

		copied += loff;
	}

	list_splice_tail(&new_pages, &f->list);

	/* 2-byte align the lrecoff */
	if (loff % 2)
		loff++;

	f->pages += npages;
	f->recs++;
	f->lrecoff = loff;

	mutex_unlock(&f->lock);
	return 0;

err:
	mutex_unlock(&f->lock);

	list_for_each_entry_safe(spage, tmp, &new_pages, list)
		free_pages(spage, 0);

	return ret;

}
