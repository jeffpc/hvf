/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __CONSOLE_H
#define __CONSOLE_H

#include <device.h>
#include <io.h>
#include <spinlock.h>

/* line allocation size */
#define CON_LINE_ALLOC_SIZE	256

/* maximum line length */
#define CON_MAX_LINE_LEN	(CON_LINE_ALLOC_SIZE - sizeof(struct console_line))

/* maximum number of free lines to keep around */
#define CON_MAX_FREE_LINES	32

/* number of lines of console text to flush at a time */
#define CON_MAX_FLUSH_LINES	32

/* line state */
#define CON_STATE_FREE		0
#define CON_STATE_PENDING	1
#define CON_STATE_IO		2

struct console_line {
	struct list_head lines;
	u16 len;
	u16 state;
	u8 buf[0];
};

struct console {
	struct list_head consoles;
	struct virt_sys *sys;
	struct device *dev;
	spinlock_t lock;
	struct list_head write_lines;
	struct list_head read_lines;
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
