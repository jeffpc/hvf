/*
 * Copyright (c) 2007 Josef 'Jeff' Sipek
 */

#include <channel.h>
#include <console.h>
#include <directory.h>
#include <ebcdic.h>
#include <slab.h>

int vprintf(struct console *con, const char *fmt, va_list args)
{
	int ret;
	char buf[128];

	ret = vsnprintf(buf, 128, fmt, args);
	if (ret) {
		ascii2ebcdic((u8 *) buf, ret);
		con_write(con, (u8 *) buf, ret);
	}

	return ret;
}

int snprintf(char *buf, int len, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	len = vsnprintf(buf, len, fmt, args);
	va_end(args);

	return len;
}

int con_printf(struct console *con, const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
	r = vprintf(con, fmt, args);
	va_end(args);

	return r;
}
