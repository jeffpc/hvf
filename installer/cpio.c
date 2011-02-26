#include <stdio.h>
#include <string.h>

typedef unsigned long long u64;
typedef signed long long s64;

typedef unsigned int u32;
typedef signed int s32;

typedef unsigned short u16;
typedef signed short s16;

typedef unsigned char u8;
typedef signed char s8;

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
	char	fn[8];
	char	ft[8];
	int	lrecl;
	int	text;
};

struct table table[] = {
	{"HVF     ", "DIRECT  ", 80, 1},
	{"SYSTEM  ", "CONFIG  ", 80, 1},
	{""        , ""        , -1, -1},
};

static u32 getnumber(u8 *data, int digits)
{
	u32 ret = 0;

	for(;digits; digits--, data++)
		ret = (ret * 8) + (*data - '0');

	return ret;
}

void readcard(u8 *buf)
{
	int i;

	for(i=0; i<80; i++)
		buf[i] = getchar();
}

u8 dasd_buf[1024 * 1024];
void load()
{
	struct cpio_hdr *hdr = (void*) dasd_buf;
	int fill;

	u32 filesize;
	u32 namesize;

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

		printf("processing '%s' of %u bytes\n", hdr->data, filesize);

		// FIXME: save the filename

		fill -= (sizeof(struct cpio_hdr) + namesize);
		memmove(hdr, hdr->data + namesize, fill);

		/* read the entire file into storage (assuming it's <= 1MB) */
		while(fill < filesize) {
			readcard(dasd_buf + fill);
			fill += 80;
		}

		printf("write %u bytes to dasd\n", filesize);

		// FIXME: do dasd io

		fill -= filesize;
		memmove(dasd_buf, dasd_buf + filesize, fill);
	}
}

void main()
{
	load();
}
