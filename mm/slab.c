/*
 * Copyright (c) 2007 Josef 'Jeff' Sipek
 */

#include <buddy.h>
#include <slab.h>
#include <page.h>

static struct slab *generic[5];

/*
 * Create slab caches for 16, 32, 64, 128, and 256 byte allocations
 */
int init_slab()
{
	generic[0] = create_slab(16);
	if (!generic[0])
		goto out_err;

	generic[1] = create_slab(32);
	if (!generic[1])
		goto out_err;

	generic[2] = create_slab(64);
	if (!generic[2])
		goto out_err;

	generic[3] = create_slab(128);
	if (!generic[3])
		goto out_err;

	generic[4] = create_slab(256);
	if (!generic[4])
		goto out_err;

	return 0;

out_err:
	free_slab(generic[0]);
	free_slab(generic[1]);
	free_slab(generic[2]);
	free_slab(generic[3]);
	free_slab(generic[4]);

	return -ENOMEM;
}

struct slab *create_slab(u16 objsize)
{
	struct page *page;
	struct slab *slab;

	page = alloc_pages(0);
	if (!page)
		return NULL;

	slab = page_to_addr(page);
	memset(slab, 0, PAGE_SIZE);

	slab->next = NULL;
	slab->objsize = objsize;
	slab->count = 8 * (PAGE_SIZE - sizeof(struct slab)) / (8 * objsize + 1);
	slab->used = 0;

	return slab;
}

void free_slab(struct slab *slab)
{
	struct slab *next;

	if (!slab)
		return;

	for(next = slab->next; slab; slab = next) {
		/*
		 * TODO: debugging support should check if slab->used != 0
		 */
		free_pages(slab, 0);
	}
}

static inline void *__alloc_slab_obj_newpage(struct slab *slab)
{
	struct page *page;

	page = alloc_pages(0);
	if (!page)
		return ERR_PTR(-ENOMEM);

	slab->next = page_to_addr(page);

	return slab->next;
}

void *alloc_slab_obj(struct slab *slab)
{
	int objidx;
	u8 *bits;
	u8 mask;

	if (!slab)
		return NULL;

	/*
	 * Find the first slab page that has unused objects
	 */
	while((slab->used == slab->count) && slab->next)
		slab = slab->next;

	if ((slab->used == slab->count) && !slab->next) {
		slab = __alloc_slab_obj_newpage(slab);
		if (PTR_ERR(slab))
			return NULL;
	}

	/* found a page */
	for (objidx = 0; objidx < slab->count; objidx++) {
		bits = ((u8 *) slab) + sizeof(struct slab) + (objidx/8);

		mask = 1 << (7 - (objidx % 8));

		if (*bits & mask)
			continue;

		slab->used++;
		*bits |= mask;

		return ((u8*) slab) + slab->startoff + slab->objsize * objidx;
	}

	/* SOMETHING STRANGE HAPPENED */
	return NULL;
}

void free_slab_obj(void *ptr)
{
	struct slab *slab;
	int objidx;
	u8 *bits;

	/* get the slab object ptr */
	slab = (struct slab *) (((u64) ptr) & ~0xfff);

	/* calculate the object number */
	objidx = (((u64) ptr) - ((u64) slab) - slab->startoff) / slab->objsize;

	/* update the bitmap */
	bits = ((u8 *) slab) + sizeof(struct slab) + (objidx/8);
	*bits &= ~(1 << (7 - (objidx % 8)));

	if (--slab->used) {
		/* free the page? */
	}
}

void *malloc(int size)
{
	if (size <= 16)
		return alloc_slab_obj(generic[0]);
	if (size <= 32)
		return alloc_slab_obj(generic[1]);
	if (size <= 64)
		return alloc_slab_obj(generic[2]);
	if (size <= 128)
		return alloc_slab_obj(generic[3]);
	if (size <= 256)
		return alloc_slab_obj(generic[4]);
	return NULL;
}

void free(void *ptr)
{
	free_slab_obj(ptr);
}
