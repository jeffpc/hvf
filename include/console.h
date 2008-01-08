#ifndef __CONSOLE_H
#define __CONSOLE_H

#include <device.h>
#include <io.h>
#include <spinlock.h>

#define CON_LINE_ALLOC_SIZE	256
#define CON_MAX_LINE_LEN	(CON_LINE_ALLOC_SIZE - sizeof(struct console_line))
#define CON_MAX_LINES		64

struct console_line {
	struct io_op ioop;
	struct ccw ccw;
	u16 len;
	u16 used;
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
