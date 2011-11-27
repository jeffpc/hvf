/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <slab.h>
#include <util.h>

char *strdup(char *s, int flags)
{
	char *d;
	int len;

	for(d=s; *d; d++)
		;

	len = d - s;

	d = malloc(sizeof(char)*len, flags);
	if (!d)
		return NULL;

	memcpy(d, s, len*sizeof(char));

	return d;
}

int hex(char *a, char *b, u64 *out)
{
	u64 val;
	char c;

	if (a>=b)
		return 1;

	val = 0;

	while(a<b) {
		c = *a;

		if ((c >= '0') && (c <= '9'))
			val = (val << 4) | (*a - '0');
		else if ((c >= 'A') && (c <= 'F'))
			val = (val << 4) | (*a - 'A' + 10);
		else if ((c >= 'a') && (c <= 'f'))
			val = (val << 4) | (*a - 'a' + 10);
		else
			return 1;

		a++;
	}

	*out = val;

	return 0;
}

int bcd2dec(u64 val, u64 *out)
{
	u64 v;
	u64 scale;

	for(v=0,scale=1; val; scale*=10, val>>=4) {
		u64 x = val & 0xf;

		if (x>=10)
			return 1;

		v += (x*scale);
	}

	*out = v;

	return 0;
}
