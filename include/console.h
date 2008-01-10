#ifndef __CONSOLE_H
#define __CONSOLE_H

#include <device.h>
#include <io.h>
#include <spinlock.h>

/* line allocation size */
#define CON_LINE_ALLOC_SIZE	256

/* maximum line length */
#define CON_MAX_LINE_LEN	(CON_LINE_ALLOC_SIZE - sizeof(struct console_line))

/* maximum number of lines to buffer */
#define CON_MAX_LINES		64

/* number of lines of console text to flush at a time */
#define CON_MAX_FLUSH_LINES	8

/* line state */
#define CON_STATE_FREE		0
#define CON_STATE_PENDING	1
#define CON_STATE_IO		2

struct console_line {
	u16 len;
	u16 state;
	u8 buf[0];
};

struct console {
	struct device *dev;
	spinlock_t lock;
	int nlines;
	struct console_line *lines[CON_MAX_LINES];
};

extern int con_write(struct console *con, u8 *buf, int len);

#endif
