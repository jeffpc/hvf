/*
 * (C) Copyright 2009  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <errno.h>
#include <string.h>
#include <digest.h>

static void sha1_init(struct digest_ctx *ctx)
{
	ctx->sha1.H[0] = 0x67452301;
	ctx->sha1.H[1] = 0xEFCDAB89;
	ctx->sha1.H[2] = 0x98BADCFE;
	ctx->sha1.H[3] = 0x10325476;
	ctx->sha1.H[4] = 0xC3D2E1F0;
	ctx->sha1.mbl = 0;
	ctx->sha1.buflen = 0;
}

#define DIGEST_LOOP(inst,type,ctx,ptr,len)	do { \
		asm volatile( \
			"lr	%%r0,%0\n"	/* the function code */ \
			"lr	%%r1,%1\n"	/* the param block */	\
			"lr	%%r2,%2\n"	/* data address */	\
			"lr	%%r3,%3\n"	/* data length */	\
			"0:" inst ",0,%%r2\n"	/* hash! */		\
			"bnz	0b\n"		/* not done yet */	\
			: /* output */					\
			: /* input */					\
			  "d" (type),					\
			  "a" (ctx),					\
			  "a" (ptr),					\
			  "d" (len)					\
			: /* clobbered */				\
			  "cc", "r0", "r1", "r2", "r3", "memory"	\
		);							\
	} while(0)

#define KIMD(type,ctx,ptr,len)	DIGEST_LOOP(".insn rre,0xb93e0000", type, ctx, ptr, len)
#define KLMD(type,ctx,ptr,len)	DIGEST_LOOP(".insn rre,0xb93f0000", type, ctx, ptr, len)

static void __sha1_update(struct digest_ctx *ctx, u8 *data, u64 len, int final)
{
	if (ctx->sha1.buflen) {
		/* we have something in the context buffer */
		u8 copy = min_t(u64, 64-ctx->sha1.buflen, len);

		memcpy(ctx->sha1.buf + ctx->sha1.buflen, data, copy);

		ctx->sha1.buflen += copy;
		data += copy;
		len -= copy;

		if (ctx->sha1.buflen == 64) {
			KIMD(ctx->type, ctx->sha1.H, ctx->sha1.buf, ctx->sha1.buflen);

			ctx->sha1.mbl += (ctx->sha1.buflen << 3);
			ctx->sha1.buflen = 0;
		} else if (final) {
			/* this will be the last call, but since we don't
			 * have a full context buffer, it means that we
			 * didn't have enough data to fill it in...so just
			 * use KLMD on the internal buffer!
			 */
			data = ctx->sha1.buf;
			len = ctx->sha1.buflen;
		}
	}

	if (!final && (len % 64)) {
		u8 tail = len % 64;

		len -= tail;
		memcpy(ctx->sha1.buf, data + len, tail);
		ctx->sha1.buflen = tail;
	}

	ctx->sha1.mbl += (len << 3);

	if (final)
		KLMD(ctx->type, ctx->sha1.H, data, len);
	else
		KIMD(ctx->type, ctx->sha1.H, data, len);
}

static void sha1_update(struct digest_ctx *ctx, u8 *data, u64 len)
{
	__sha1_update(ctx, data, len, 0);
}

static void sha1_finalize(struct digest_ctx *ctx, u8 *data, u64 len)
{
	__sha1_update(ctx, data, len, 1);
}

struct digest_info digests[] = {
	[DIGEST_QUERY] = {
		.init = NULL,
		.update = NULL,
		.finalize = NULL,
	},
	[DIGEST_SHA1] = {
		.init     = sha1_init,
		.update   = sha1_update,
		.finalize = sha1_finalize,
	},
};

int digest_init_ctx(int dt, struct digest_ctx *ctx)
{
	if ((dt <= DIGEST_QUERY) || (dt > DIGEST_SHA1))
		return -EINVAL;

	ctx->type = dt;
	digests[dt].init(ctx);

	return 0;
}

void digest_update_ctx(struct digest_ctx *ctx, u8 *data, u64 len)
{
	digests[ctx->type].update(ctx, data, len);
}

void digest_finalize_ctx(struct digest_ctx *ctx, u8 *data, u64 len)
{
	digests[ctx->type].finalize(ctx, data, len);
}
