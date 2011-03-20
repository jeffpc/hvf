/*
 * Copyright (c) 2011 Josef 'Jeff' Sipek
 */
#include "loader.h"
#include <string.h>
#include <ebcdic.h>
#include <list.h>

union adt_u {
	struct ADT adt;
	char buf[4096];
};

struct block_map {
	struct list_head list;

	/* key */
	u8 fn[8]; /* EBCDIC */
	u8 ft[8]; /* EBCDIC */
	u8 level; /* 0 = data */
	u32 blk_no;

	/* value */
	u32 lba;
	void *buf;
};

static union adt_u *adt;
static struct FST directory;

static LIST_HEAD(block_map);

/* borrowed from Linux */
#define container_of(ptr, type, member) ({                      \
         const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
         (type *)( (char *)__mptr - offsetof(type,member) );})

/* borrowed from Linux */
#define offsetof(type, member) __builtin_offsetof(type,member)

#define DIRECTOR_FN ((u8*) "\x00\x00\x00\x01\x00\x00\x00\x00")
#define DIRECTOR_FT ((u8*) "\xc4\xc9\xd9\xc5\xc3\xe3\xd6\xd9")
#define ALLOCMAP_FN ((u8*) "\x00\x00\x00\x02\x00\x00\x00\x00")
#define ALLOCMAP_FT ((u8*) "\xc1\xd3\xd3\xd6\xc3\xd4\xc1\xd7")

static struct block_map *block_map_find(u8 *fn, u8 *ft, u8 level, u32 blk_no)
{
	struct block_map *cur;

	list_for_each_entry(cur, &block_map, list) {
		if (!memcmp(fn, cur->fn, 8) &&
		    !memcmp(ft, cur->ft, 8) &&
		    (level == cur->level) &&
		    (blk_no == cur->blk_no))
			return cur;
	}

	return NULL;
}

static void block_map_add(u8 *fn, u8 *ft, u8 level, u32 blk_no, u32 lba)
{
	struct block_map *map;

	map = block_map_find(fn, ft, level, blk_no);
	if (map) {
		if (map->lba != lba)
			die();

		return;
	}

	map = malloc(sizeof(struct block_map));
	if (!map)
		die();

	memcpy(map->fn, fn, 8);
	memcpy(map->ft, ft, 8);
	map->level = level;
	map->blk_no = blk_no;

	map->lba = lba;
	map->buf = NULL;

	list_add(&map->list, &block_map);
}

static void *read_file_blk(u8 *fn, u8 *ft, u8 level, u32 blk)
{
	struct block_map *map;

	map = block_map_find(fn, ft, level, blk);
	if (!map)
		die();

	if (map->buf)
		return map->buf;

	map->buf = malloc(adt->adt.DBSIZ);
	if (!map->buf)
		die();

	read_blk(map->buf, map->lba);

	return map->buf;
}

/*
 * fills in *fst with existing file info and returns 0, or if file doesn't
 * exist, returns -1
 */
int find_file(char *fn, char *ft, struct FST *fst)
{
	struct FST *FST;
	u8 FN[8];
	u8 FT[8];
	int rec;
	int blk;

	memcpy(FN, fn, 8);
	memcpy(FT, ft, 8);
	ascii2ebcdic(FN, 8);
	ascii2ebcdic(FT, 8);

	for(blk=0; blk<directory.ADBC; blk++) {
		FST = read_file_blk(DIRECTOR_FN, DIRECTOR_FT, 0, blk);

		for(rec=0; rec<(adt->adt.DBSIZ / directory.LRECL); rec++) {
			if ((!memcmp(FST[rec].FNAME, FN, 8)) &&
			    (!memcmp(FST[rec].FTYPE, FT, 8))) {
				memcpy(fst, &FST[rec], sizeof(struct FST));
				return 0;
			}
		}
	}

	return -1;
}

static int file_blocks_at_level(u32 ADBC, int level)
{
	const u32 ptrs_per_block = adt->adt.DBSIZ / 4;
	int blks;
	int i;

	blks = 1;
	for(i=0; i<level; i++)
		blks *= ptrs_per_block;
	blks = (ADBC + blks - 1) / blks;

	return blks;
}

static void __read_file(struct FST *fst)
{
	const u32 ptrs_per_block = adt->adt.DBSIZ / 4;
	u32 *blk_ptrs;
	int level;

	int blocks;
	int i,j;

	if (!fst[0].NLVL) {
		block_map_add(fst->FNAME, fst->FTYPE, 0, 0, fst->FOP);
		return;
	}

	/* there are pointer blocks, let's read them in and then
	 * follow each pointer
	 */

	block_map_add(fst->FNAME, fst->FTYPE, fst->NLVL, 0, fst->FOP);

	/* for each level of pointers... */
	for(level=fst->NLVL; level>0; level--) {
		blocks = file_blocks_at_level(fst->ADBC, level);

		if (level == fst->NLVL && blocks != 1)
			die();

		/* read in each block */
		for(i=0; i<blocks; i++) {
			blk_ptrs = read_file_blk(fst->FNAME,
						 fst->FTYPE,
						 level,
						 i);

			/* and add each pointer to the next level */
			for(j=0; j<ptrs_per_block; j++) {
				block_map_add(fst->FNAME,
					      fst->FTYPE,
					      level-1,
					      (i*ptrs_per_block) + j,
					      blk_ptrs[j]);
			}
		}
	}
}

void mount_fs()
{
	struct FST *fst;

	adt = malloc(sizeof(union adt_u));

	read_blk(adt, EDF_LABEL_BLOCK_NO);

	if ((adt->adt.IDENT != __ADTIDENT) ||
	    (adt->adt.DBSIZ != EDF_SUPPORTED_BLOCK_SIZE) ||
	    (adt->adt.OFFST != 0) ||
	    (adt->adt.FSTSZ != sizeof(struct FST)))
		die();

	block_map_add(DIRECTOR_FN, DIRECTOR_FT, 0, 0, adt->adt.DOP);

	fst = read_file_blk(DIRECTOR_FN, DIRECTOR_FT, 0, 0);

	if (memcmp(fst[0].FNAME, DIRECTOR_FN, 8) ||
	    memcmp(fst[0].FTYPE, DIRECTOR_FT, 8) ||
	    (fst[0].RECFM != FSTDFIX) ||
	    memcmp(fst[1].FNAME, ALLOCMAP_FN, 8) ||
	    memcmp(fst[1].FTYPE, ALLOCMAP_FT, 8) ||
	    (fst[1].RECFM != FSTDFIX))
		die();

	memcpy(&directory, fst, sizeof(struct FST));

	__read_file(&fst[0]); /* the directory */
	__read_file(&fst[1]); /* the alloc map */
}
