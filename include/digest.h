/*
 * Copyright (c) 2007-2010 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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
