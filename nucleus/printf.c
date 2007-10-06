/*
 * Copyright (c) 2007 Josef 'Jeff' Sipek
 */

int vprintf(const char *fmt, va_list args)
{
	static char buf[80];
	int ret;

	ret = vsnprintf(buf, 80, fmt, args);

	return ret;
}

/*
 * Logic borrowed from Linux's printk
 */
int printf(const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
	r = vprintf(fmt, args);
	va_end(args);

	return r;
}
