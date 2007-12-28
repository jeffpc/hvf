#ifndef __SLAB_H
#define __SLAB_H

#include <list.h>

struct slab {
	u32 magic;
	u16 objsize;
	u16 startoff;
	struct list_head slabs;
	struct list_head slab_pages;
	u16 count;
	u16 used;
	u8 bitmap[0];
} __attribute((packed))__;

extern int init_slab();

extern struct slab *create_slab(u16 objsize, u8 align);
extern void free_slab(struct slab *slab);

extern void *malloc(int size);
extern void free(void *ptr);

#endif
