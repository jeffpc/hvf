/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __DIGEST_H
#define __DIGEST_H

#define DIGEST_QUERY		0
#define DIGEST_SHA1		1

struct digest_ctx_sha1 {
	u32 H[5];
	u64 mbl;
	u8 buflen;
	u8 buf[64];
} __attribute__((packed,aligned(8)));

struct digest_ctx {
	u8 type;
	u8 __reserved[7];
	union {
		struct digest_ctx_sha1 sha1;
	};
};

struct digest_info {
	void (*init)(struct digest_ctx *);
	void (*update)(struct digest_ctx *, u8 *, u64);
	void (*finalize)(struct digest_ctx *, u8 *, u64);
};

extern int digest_init_ctx(int dt, struct digest_ctx *ctx);
extern void digest_update_ctx(struct digest_ctx *ctx, u8 *data, u64 len);
extern void digest_finalize_ctx(struct digest_ctx *ctx, u8 *data, u64 len);

#endif
