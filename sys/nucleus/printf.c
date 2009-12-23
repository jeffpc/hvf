/*
 * Copyright (c) 2007 Josef 'Jeff' Sipek
 */

#include <channel.h>
#include <console.h>
#include <directory.h>
#include <ebcdic.h>
#include <slab.h>
#include <clock.h>
#include <sched.h>
#include <vsprintf.h>
#include <stdarg.h>

int vprintf(struct console *con, const char *fmt, va_list args)
{
	struct datetime dt;
	char buf[128];
	int off = 0;
	int ret;

	if (!con->sys || (con->sys && con->sys->print_ts)) {
		memset(&dt, 0, sizeof(dt));
		ret = get_parsed_tod(&dt);
		off = snprintf(buf, 128, "%02d:%02d:%02d ", dt.th, dt.tm,
			       dt.ts);
	}

	ret = vsnprintf(buf+off, 128-off, fmt, args);
	if (ret) {
		ascii2ebcdic((u8 *) buf, off+ret);
		con_write(con, (u8 *) buf, off+ret);
	}

	return ret;
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
