/*
 * Copyright (c) 2012 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#ifndef __CIRCBUF_H
#define __CIRCBUF_H

struct circbuf {
	u32 start;			/* the oldest entry idx */
	u32 filled;			/* the number of entries */
	u32 size;			/* the size of each entry */
	u32 count;			/* the max number of entries */
	u8 data[0];
};

#define CIRCBUF(name, n, s)	struct circbuf name; u8 __circbuf_##name[(n)*sizeof(s)]

#define init_circbuf(c, t, n)	__init_circbuf((c), sizeof(t), (n))
static inline void __init_circbuf(struct circbuf *c, u32 size, u32 count)
{
	c->start = 0;
	c->filled = 0;
	c->size = size;
	c->count = count;
}

static inline void insert_circbuf(struct circbuf *c, void *data)
{
	u32 idx;

	if (c->filled == c->count) {
		idx = c->start;
		c->start = (c->start+1) % c->count;
	} else {
		idx = (c->start + c->filled) % c->count;
		c->filled++;
	}

	memcpy(c->data + (idx * c->size), data, c->size);
}

static inline int remove_circbuf(struct circbuf *c, void *data)
{
	if (!c->filled)
		return 0;

	memcpy(data, c->data + (c->start * c->size), c->size);

	c->start = (c->start+1) % c->count;
	c->filled--;
	return 1;
}

static inline int pending_circbuf(struct circbuf *c)
{
	return c->filled != 0;
}

#endif
