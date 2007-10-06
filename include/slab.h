#ifndef __SLAB_H
#define __SLAB_H

struct slab {
	struct slab *next;
	u16 objsize;
	u16 count;
	u16 used;
	u16 startoff;
} __attribute((packed))__;

extern int init_slab();

extern struct slab *create_slab(u16 objsize);
extern void free_slab(struct slab *slab);

#endif
