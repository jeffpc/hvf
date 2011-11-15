/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __CONSOLE_H
#define __CONSOLE_H

#include <device.h>
#include <io.h>
#include <spinlock.h>

#define CONSOLE_LINE_LEN	160

struct console {
	struct list_head consoles;
	struct virt_sys *sys;
	struct device *dev;

	struct spool_file *wlines;
	struct spool_file *rlines;

	u8 *bigbuf;
};

extern int console_interrupt(struct device *dev, struct irb *irb);
extern struct console* start_oper_console(void);
extern void* console_enable(struct device *dev);
extern int con_read_pending(struct console *con);
extern int con_read(struct console *con, u8 *buf, int size);
extern int con_write(struct console *con, u8 *buf, int len);
extern void for_each_console(void (*f)(struct console *con));
extern struct console* find_console(struct device *dev);

#endif
