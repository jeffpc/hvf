/*
 * Copyright (c) 2007 Josef 'Jeff' Sipek
 */

#include <channel.h>
#include <console.h>
#include <ebcdic.h>
#include <slab.h>

int vprintf(const char *fmt, va_list args)
{
	int ret;
	char buf[128];

	ret = vsnprintf(buf, 128, fmt, args);
	if (ret) {
		ascii2ebcdic((u8 *) buf, ret);
		oper_con_write((u8 *) buf, ret);
	}

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
