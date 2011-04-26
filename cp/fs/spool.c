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

	INIT_LIST_HEAD(&new_pages);
	npages = 0;

	BUG_ON(sizeof(struct spool_rec) != 2);

	mutex_lock(&f->lock);

	left = SPOOL_DATA_SIZE - f->lrecoff;
	rlen = len + sizeof(struct spool_rec);
	copied = 0;

	/* try to fill up the last page */
	if (f->pages && left) {
		spage = list_last_entry(&f->list, struct spool_page, list);
		rec = (struct spool_rec*) (spage->data + f->lrecoff);

		switch(left) {
			case 1:
				spage->data[f->lrecoff] = len >> 8;
				copied = 1;
				loff = SPOOL_DATA_SIZE;
				break;
			case 2:
				rec->len = len;
				copied = 2;
				loff = SPOOL_DATA_SIZE;
				break;
			default:
				rec->len = len;
				memcpy(rec->data, buf, min_t(u32, len, left-2));
				copied = 2 + min_t(u32, len, left-2);
				loff = f->lrecoff + copied;
				break;
		}
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

		switch(copied) {
			case 0:
				/* nothing was copied */
				rec->len = len;
				memcpy(rec->data, buf, min_t(u32, len, left-2));
				loff = 2 + min_t(u32, len, left-2);
				break;
			case 1:
				/* half of the length was copied */
				spage->data[0] = len & 0xff;
				rec = (struct spool_rec*) (spage->data - 1);
				memcpy(rec->data, buf, min_t(u32, len, left-1));
				loff = 1 + min_t(u32, len, left-1);
				break;
			default:
				/* the length and maybe some data were copied */
				memcpy(spage, buf-copied+2, min(rlen-copied, left));
				loff = min(rlen-copied, left);
				break;
		}

		copied += loff;
	}

	list_splice_tail(&new_pages, &f->list);

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
