#ifndef __SLAB_H
#define __SLAB_H

#include <list.h>
#include <spinlock.h>

struct slab {
	u32 magic;			/* magic */
	spinlock_t lock;		/* lock to protect entire slab */
	struct list_head slab_pages;	/* list of pages */
	struct slab *first;		/* pointer to first slab page */
	u16 objsize;			/* effective object size */
	u16 startoff;			/* first object offset */
	u16 count;			/* number of objects in this page */
	u16 used;			/* number of used objects */
	u8 bitmap[0];			/* allocation bitmap */
} __attribute((packed))__;

extern int init_slab();

extern struct slab *create_slab(u16 objsize, u8 align);
extern void free_slab(struct slab *slab);

extern void *malloc(int size);
extern void free(void *ptr);

#endif
