/*
 * Copyright (c) 2011 Josef 'Jeff' Sipek
 */
#include "loader.h"
#include <string.h>
#include <ebcdic.h>

struct cpio_hdr {
	u8	magic[6];
	u8	dev[6];
	u8	ino[6];
	u8	mode[6];
	u8	uid[6];
	u8	gid[6];
	u8	nlink[6];
	u8	rdev[6];
	u8	mtime[11];
	u8	namesize[6];
	u8	filesize[11];
	u8	data[0];
};

struct table {
	char	*arch;
	char	fn[8];
	char	ft[8];
	int	lrecl;
	int	text;
};

static struct table table[] = {
	{"hvf.directory",	"HVF     ", "DIRECT  ", 80, 1},
	{"hvf.config",		"SYSTEM  ", "CONFIG  ", 80, 1},
//	{"hvf",			"HVF     ", "ELF     ", 4096, 0},
	{"",			""        , ""        , -1, -1},
};

static void save_file(struct table *te, int filesize, u8 *buf)
{
	struct FST fst;
	int ret;
	int rec;

	ret = find_file(te->fn, te->ft, &fst);
	if (!ret) {
		wto("File '");
		wto(te->fn);
		wto("' already exists on the device.\n");
		wto("The device has been left unmodified.\n");
		die();
	}

	ret = create_file(te->fn, te->ft, te->lrecl, &fst);
	if (ret) {
		wto("Could not create file '");
		wto(te->fn);
		wto("'.\n");
		die();
	}

	if (te->text)
		ascii2ebcdic(buf, filesize);

	for(rec=0; rec<(filesize/te->lrecl); rec++) {
		char pbuf[100];
		snprintf(pbuf, 100, "rec %03x at %p\n", rec,
			 buf + (rec * te->lrecl));
		wto(pbuf);
		append_record(&fst, buf + (rec * te->lrecl));
	}

	if (filesize % te->lrecl) {
		u8 buf2[te->lrecl];

		memset(buf2, 0, te->lrecl);
		memcpy(buf2, buf + (rec * te->lrecl), te->lrecl);

		append_record(&fst, buf2);
	}
}

static u32 getnumber(u8 *data, int digits)
{
	u32 ret = 0;

	for(;digits; digits--, data++)
		ret = (ret * 8) + (*data - '0');

	return ret;
}

static void readcard(u8 *buf)
{
	static int eof;
	int ret;
	struct ccw ccw;

	if (eof)
		return;

	ccw.cmd   = 0x02;
	ccw.flags = 0;
	ccw.count = 80;
	ccw.addr  = ADDR31(buf);

	ORB.param = 0x12345678,
	ORB.f     = 1,
	ORB.lpm   = 0xff,
	ORB.addr  = ADDR31(&ccw);

	ret = __do_io(ipl_sch);
	if (ret == 0x01) {
		eof = 1;
		return;  // end of media
	}

	if (ret)
		die();
}

void unload_archive(void)
{
	struct cpio_hdr *hdr;
	u8 *dasd_buf;
	int save;
	int fill;
	int i;

	u32 filesize;
	u32 namesize;

	dasd_buf = malloc(2*1024*1024);
	hdr = (void*) dasd_buf;

	wto("\n");

	fill = 0;
	while(1) {
		/* read a file header */
		if (fill < sizeof(struct cpio_hdr)) {
			readcard(dasd_buf + fill);
			fill += 80;
		}

		namesize = getnumber(hdr->namesize, 6);
		filesize = getnumber(hdr->filesize, 11);

		while(namesize + sizeof(struct cpio_hdr) > fill) {
			readcard(dasd_buf + fill);
			fill += 80;
		}

		if ((namesize == 11) &&
		    !strncmp("TRAILER!!!", (char*) hdr->data, 10))
			break;

		save = 0;
		for(i=0; table[i].lrecl != -1; i++) {
			if (!strcmp((char*) hdr->data, table[i].arch)) {
				save = 1;
				break;
			}
		}

		if (save) {
			wto("processing '");
			wto((char*) hdr->data);
			wto("' => '");
			wto(table[i].fn);
			wto("'\n");
		} else {
			wto("skipping   '");
			wto((char*) hdr->data);
			wto("'\n");
		}

		fill -= (sizeof(struct cpio_hdr) + namesize);
		memmove(hdr, hdr->data + namesize, fill);

		/* read the entire file into storage (assuming it's <= 1MB) */
		while(fill < filesize) {
			readcard(dasd_buf + fill);
			fill += 80;
		}

		if (save)
			save_file(&table[i], filesize, dasd_buf);

		fill -= filesize;
		memmove(dasd_buf, dasd_buf + filesize, fill);
	}
}
