/*
 * Copyright (c) 2007 Josef 'Jeff' Sipek
 */

#include <channel.h>
#include <io.h>

u32 oper_console_ssid;

static void __start_console_io(char *buf, int len)
{
	struct io_op ioop;
	struct ccw ccw;

	ioop.handler = NULL;
	ioop.ssid = oper_console_ssid;

	memset(&ioop.orb, 0, sizeof(struct orb));
	ioop.orb.lpm = 0xff;
	ioop.orb.addr = (u32) &ccw;
	ioop.orb.f = 1;

	memset(&ccw, 0, sizeof(struct ccw));
	ccw.cmd = 0x01; //FIXME
	ccw.sli = 1;
	ccw.count = len;
	ccw.addr = (u32) buf;

	submit_io(&ioop, 0);
}

int vprintf(const char *fmt, va_list args)
{
	static char buf[80];
	int ret;

	ret = vsnprintf(buf, 80, fmt, args);
	if (ret)
		__start_console_io(buf, ret);

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
