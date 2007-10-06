/*
 * Copyright (c) 2007 Josef 'Jeff' Sipek
 */

void *memset(void *s, int c, size_t n)
{
	unsigned char *ptr = s;

	while(n--)
		*ptr++ = c;

	return s;
}

void *memcpy(void *dst, void *src, int len)
{
	unsigned char *d = dst;
	unsigned char *s = src;

	while(len--)
		*d++ = *s++;

	return dst;
}

int strnlen(const char *s, size_t maxlen)
{
	int len;
	
	for(len=0; *s && len <= maxlen; s++, len++)
		;

	return len;
}
