/*
 * Copyright (c) 2011 Josef 'Jeff' Sipek
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libgen.h>

typedef unsigned char u8;

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

struct buffer {
	struct buffer *next;
	int	len;
	u8	data[0];
};

static void storeoct(u8 *buf, int len, unsigned val)
{
	len--;

	while(val) {
		buf[len] = '0' + (val % 8);
		val /= 8;
		len--;
	}
}

static void mkhdr(struct cpio_hdr *hdr, char *fn, unsigned filesize)
{
	memset(hdr, '0', sizeof(struct cpio_hdr));

	hdr->magic[1] = '7';
	hdr->magic[3] = '7';
	hdr->magic[5] = '7';

	hdr->nlink[5] = '1';

	storeoct(hdr->namesize, 6, strlen(fn)+1);
	storeoct(hdr->filesize, 11, filesize);
}

static unsigned __text(char *fn, int lrecl)
{
	struct cpio_hdr hdr;
	struct buffer *first, *last, *cur, *tmp;
	FILE *f;
	int l;
	unsigned flen;

	if (!lrecl) {
		fprintf(stderr, "Error: zero lrecl is bogus\n");
		exit(1);
	}

	f = fopen(fn, "rb");
	if (!f) {
		fprintf(stderr, "Error: could not open file '%s'\n",
			fn);
		exit(1);
	}

	flen  = 0;
	first = NULL;
	last  = NULL;
	for(;;) {
		cur = malloc(sizeof(struct buffer) + lrecl + 1);
		if (!cur) {
			fprintf(stderr, "Error: OOM\n");
			exit(2);
		}

		if (!fgets((char*)cur->data, lrecl+1, f))
			break;

		l = strlen((char*)cur->data);

		if (l && cur->data[l-1])
			l--;

		assert(l <= lrecl);

		memset(cur->data+l, ' ', lrecl-l);

		cur->next = NULL;
		if (!first)
			first = cur;
		if (last)
			last->next = cur;
		last = cur;

		flen += lrecl;
	}

	fclose(f);

	mkhdr(&hdr, basename(fn), flen);
	fwrite(&hdr, sizeof(hdr), 1, stdout);
	fwrite(basename(fn), strlen(basename(fn))+1, 1, stdout);

	for(cur=first; cur; cur=tmp) {
		fwrite(cur->data, 1, lrecl, stdout);
		tmp = cur->next;
		free(cur);
	}

	return flen + strlen(basename(fn)) + 1 + sizeof(hdr);
}

#define BUF_LEN 4096
static unsigned __bin(char *fn)
{
	struct cpio_hdr hdr;
	struct buffer *first, *last, *cur, *tmp;
	FILE *f;
	unsigned flen;
	size_t s;

	f = fopen(fn, "rb");
	if (!f) {
		fprintf(stderr, "Error: could not open file '%s'\n",
			fn);
		exit(1);
	}

	flen  = 0;
	first = NULL;
	last  = NULL;
	for(;;) {
		cur = malloc(sizeof(struct buffer) + BUF_LEN);
		if (!cur) {
			fprintf(stderr, "Error: OOM\n");
			exit(2);
		}

		memset(cur->data, 0, BUF_LEN);
		if (!(s = fread(cur->data, 1, BUF_LEN, f)))
			break;

		cur->next = NULL;
		cur->len  = s;
		if (!first)
			first = cur;
		if (last)
			last->next = cur;
		last = cur;

		flen += s;
	}

	fclose(f);

	mkhdr(&hdr, basename(fn), flen);
	fwrite(&hdr, sizeof(hdr), 1, stdout);
	fwrite(basename(fn), strlen(basename(fn))+1, 1, stdout);

	for(cur=first; cur; cur=tmp) {
		fwrite(cur->data, 1, cur->len, stdout);
		tmp = cur->next;
		free(cur);
	}

	return flen + strlen(basename(fn)) + 1 + sizeof(hdr);
}

static void mktail(unsigned len)
{
	struct cpio_hdr hdr;

	mkhdr(&hdr, "TRAILER!!!", 0);
	fwrite(&hdr, sizeof(hdr), 1, stdout);
	printf("TRAILER!!!");

	while(len % 512) {
		fwrite("", 1, 1, stdout); // write a nul char
		len++;
	}
}

int main(int argc, char **argv)
{
	int lrecl;
	char *fn;
	int i;
	unsigned len;

	i=1;
	len=0;
	while((i+1) < argc) {
		fn = argv[i];
		i++;

		if (!strcmp(argv[i], "text")) {
			lrecl = atoi(argv[i+1]);
			i += 2;

			len += __text(fn, lrecl);
		} else if (!strcmp(argv[i], "bin")) {
			len += __bin(fn);
			i++;
		} else {
			fprintf(stderr, "Error: cannot understand type '%s'\n",
				argv[i]);
			return 1;
		}
	}

	mktail(len);

	return 0;
}
