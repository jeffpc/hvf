/*
 * Copyright (c) 2011-2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "loader.h"
#include <arch.h>
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

	/* dirty list */
	int dirty;
};

static union adt_u *adt;
static struct FST *directory;
static struct FST *allocmap;

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
			sigp_stop();

		return;
	}

	map = malloc(sizeof(struct block_map));
	if (!map)
		sigp_stop();

	memcpy(map->fn, fn, 8);
	memcpy(map->ft, ft, 8);
	map->level = level;
	map->blk_no = blk_no;

	map->lba = lba;
	map->buf = NULL;
	map->dirty = 0;

	list_add(&map->list, &block_map);
}

static void *read_file_blk(u8 *fn, u8 *ft, u8 level, u32 blk)
{
	struct block_map *map;

	map = block_map_find(fn, ft, level, blk);
	if (!map) {
		char buf[128];
		snprintf(buf,128,"%s could not find block %d at level %d\n",
			 __func__, blk, level);
		wto(buf);
		sigp_stop();
	}

	if (map->buf)
		return map->buf;

	map->buf = malloc(adt->adt.DBSIZ);
	if (!map->buf)
		sigp_stop();

	read_blk(map->buf, map->lba);

	return map->buf;
}

static void blk_set_dirty(u8 *fn, u8 *ft, u8 level, u32 blk)
{
	struct block_map *map;

	map = block_map_find(fn, ft, level, blk);
	if (!map || !map->buf)
		sigp_stop();

	map->dirty = 1;
}

void writeback_buffers()
{
	struct block_map *cur;

	wto("beginning buffer write back\n");
	list_for_each_entry(cur, &block_map, list) {
		if (!cur->dirty)
			continue;

		if (!cur->buf)
			sigp_stop();

		write_blk(cur->buf, cur->lba);
	}
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

	for(blk=0; blk<directory->ADBC; blk++) {
		FST = read_file_blk(DIRECTOR_FN, DIRECTOR_FT, 0, blk);

		for(rec=0; rec<adt->adt.NFST; rec++) {
			if ((!memcmp(FST[rec].FNAME, FN, 8)) &&
			    (!memcmp(FST[rec].FTYPE, FT, 8))) {
				memcpy(fst, &FST[rec], sizeof(struct FST));
				return 0;
			}
		}
	}

	return -1;
}

static void update_directory(struct FST *fst)
{
	struct FST *FST;
	int rec;
	int blk;

	for(blk=0; blk<directory->ADBC; blk++) {
		FST = read_file_blk(DIRECTOR_FN, DIRECTOR_FT, 0, blk);

		for(rec=0; rec<adt->adt.NFST; rec++) {
			if ((!memcmp(FST[rec].FNAME, fst->FNAME, 8)) &&
			    (!memcmp(FST[rec].FTYPE, fst->FTYPE, 8))) {
				memcpy(&FST[rec], fst, sizeof(struct FST));
				blk_set_dirty(DIRECTOR_FN, DIRECTOR_FT, 0, blk);
				return;
			}
		}
	}

	sigp_stop();
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
	const u32 ptrs_per_block = adt->adt.DBSIZ / fst->PTRSZ;
	u32 *blk_ptrs;
	int level;

	int blocks;
	int i,j;

	if (!fst->NLVL) {
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
			sigp_stop();

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

int create_file(char *fn, char *ft, int lrecl, struct FST *fst)
{
	/* first, fill in the FST */
	memset(fst, 0, sizeof(struct FST));

	memcpy(fst->FNAME, fn, 8);
	memcpy(fst->FTYPE, ft, 8);

	ascii2ebcdic(fst->FNAME, 8);
	ascii2ebcdic(fst->FTYPE, 8);

	fst->FMODE[0] = '\xc1'; // EBCDIC 'A'
	fst->FMODE[1] = '\xf1'; // EBCDIC '1'
	fst->RECFM    = FSTDFIX; // fixed record size
	fst->LRECL    = lrecl;
	fst->PTRSZ    = 4;

	append_record(directory, (u8*) fst);

	return 0;
}

static u32 __get_free_block()
{
	u32 blk;
	u8 *buf;
	u32 i;
	u32 bit;

	for(blk=0; blk<allocmap->ADBC; blk++) {
		buf = read_file_blk(allocmap->FNAME, allocmap->FTYPE, 0, blk);

		for(i=0; i<adt->adt.DBSIZ; i++) {
			if (buf[i] == 0xff)
				continue;

			if ((buf[i] & 0x80) == 0) { bit = 0; goto found; }
			if ((buf[i] & 0x40) == 0) { bit = 1; goto found; }
			if ((buf[i] & 0x20) == 0) { bit = 2; goto found; }
			if ((buf[i] & 0x10) == 0) { bit = 3; goto found; }
			if ((buf[i] & 0x08) == 0) { bit = 4; goto found; }
			if ((buf[i] & 0x04) == 0) { bit = 5; goto found; }
			if ((buf[i] & 0x02) == 0) { bit = 6; goto found; }
			if ((buf[i] & 0x01) == 0) { bit = 7; goto found; }

			continue; /* this is not necessary since we check
				     for 0xff right away, but GCC really
				     likes to complain about possibly
				     uninitialized use of 'bit' */

found:
			buf[i] |= (0x80 >> bit);

			blk_set_dirty(allocmap->FNAME, allocmap->FTYPE, 0, blk);

			return ((blk * adt->adt.DBSIZ * 8) + (i * 8) + bit) + 1;
		}
	}

	sigp_stop();
	return 0;
}

static void __append_block(struct FST *fst)
{
	struct block_map *map;
	u32 *buf;
	u32 lba, prevlba;
	u32 blk;
	u8 lvl;

	/* no data blocks yet */
	if (!fst->ADBC) {
		fst->ADBC = 1;
		fst->NLVL = 0;
		fst->FOP  = __get_free_block();

		block_map_add(fst->FNAME, fst->FTYPE, 0, 0, fst->FOP);
		return;
	}

	prevlba = 0; /* make gcc not warn about uninit variable */
	for(lvl=0; lvl<=fst->NLVL; lvl++, prevlba=lba) {
		blk = file_blocks_at_level(fst->ADBC+1, lvl);

		map = block_map_find(fst->FNAME, fst->FTYPE, lvl, blk-1);
		if (map) {
			int x;

			if (!lvl)
				sigp_stop();

			buf = read_file_blk(fst->FNAME, fst->FTYPE, lvl,
					    blk-1);
			blk_set_dirty(fst->FNAME, fst->FTYPE, lvl, blk-1);

			x = file_blocks_at_level(fst->ADBC+1, lvl-1) %
				(adt->adt.DBSIZ / 4);
			buf[x-1] = prevlba;

			fst->ADBC++;
			return;
		}

		lba = __get_free_block();

		block_map_add(fst->FNAME, fst->FTYPE, lvl, blk-1, lba);

		if (!lvl)
			continue;

		buf = read_file_blk(fst->FNAME, fst->FTYPE, lvl, blk-1);
		blk_set_dirty(fst->FNAME, fst->FTYPE, lvl, blk-1);

		*buf = prevlba;
	}

	lba = __get_free_block();
	block_map_add(fst->FNAME, fst->FTYPE, fst->NLVL+1, 0, lba);

	buf = read_file_blk(fst->FNAME, fst->FTYPE, fst->NLVL+1, 0);
	blk_set_dirty(fst->FNAME, fst->FTYPE, fst->NLVL+1, 0);

	buf[0] = fst->FOP;
	buf[1] = prevlba;

	fst->FOP = lba;

	fst->NLVL++;
	fst->ADBC++;
}

void append_record(struct FST *fst, u8 *buf)
{
	u32 foff;
	u32 blk;
	u32 off;
	u32 rem;

	u8 *dbuf;

	if (fst->RECFM != FSTDFIX)
		sigp_stop();

	foff = fst->AIC * fst->LRECL;

	blk = foff / adt->adt.DBSIZ;
	off = foff % adt->adt.DBSIZ;
	rem = (fst->ADBC * adt->adt.DBSIZ) - foff;

	/* need to add another block */
	if (rem < fst->LRECL)
		__append_block(fst);

	dbuf = read_file_blk(fst->FNAME, fst->FTYPE, 0, blk);

	if (!rem || (rem >= fst->LRECL)) {
		memcpy(dbuf + off, buf, fst->LRECL);
		blk_set_dirty(fst->FNAME, fst->FTYPE, 0, blk);
	} else {
		memcpy(dbuf + off, buf, rem);
		blk_set_dirty(fst->FNAME, fst->FTYPE, 0, blk);

		dbuf = read_file_blk(fst->FNAME, fst->FTYPE, 0, blk+1);
		memcpy(dbuf, buf + rem, fst->LRECL - rem);
		blk_set_dirty(fst->FNAME, fst->FTYPE, 0, blk+1);
	}

	fst->AIC++;

	update_directory(fst);
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
		sigp_stop();

	block_map_add(DIRECTOR_FN, DIRECTOR_FT, 0, 0, adt->adt.DOP);

	fst = read_file_blk(DIRECTOR_FN, DIRECTOR_FT, 0, 0);

	if (memcmp(fst[0].FNAME, DIRECTOR_FN, 8) ||
	    memcmp(fst[0].FTYPE, DIRECTOR_FT, 8) ||
	    (fst[0].RECFM != FSTDFIX) ||
	    memcmp(fst[1].FNAME, ALLOCMAP_FN, 8) ||
	    memcmp(fst[1].FTYPE, ALLOCMAP_FT, 8) ||
	    (fst[1].RECFM != FSTDFIX))
		sigp_stop();

	directory = fst;
	allocmap  = fst+1;

	__read_file(&fst[0]); /* the directory */
	__read_file(&fst[1]); /* the alloc map */
}
