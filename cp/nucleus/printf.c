/*
 * (C) Copyright 2007-2019  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
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
#include <vcpu.h>

int con_vprintf(struct virt_cons *con, const char *fmt, va_list args)
{
	struct virt_sys *sys = container_of(con, struct virt_sys, console);
	struct datetime dt;
	char buf[128];
	int off = 0;
	int ret;

	if (sys->print_ts) {
		memset(&dt, 0, sizeof(dt));
		ret = get_parsed_tod(&dt);
		off = snprintf(buf, 128, "%02d:%02d:%02d ", dt.th, dt.tm,
			       dt.ts);
	}

	ret = vsnprintf(buf+off, 128-off, fmt, args);
	if (ret) {
		if (sys->internal) {
			/* internal guests direct all console traffic to sclp */
			sclp_msg("%s", buf);
		} else {
			/* normal guests direct it to their console device */
			ascii2ebcdic((u8 *) buf, off+ret);
			//con_write(con, (u8 *) buf, off+ret);
		}
	}

	return ret;
}

int con_printf(struct virt_cons *con, const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
	r = con_vprintf(con, fmt, args);
	va_end(args);

	return r;
}
