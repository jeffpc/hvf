/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <console.h>
#include <slab.h>
#include <sched.h>
#include <directory.h>
#include <splash.h>
#include <vsprintf.h>

/*
 * List of all consoles on the system
 */
static LIST_HEAD(consoles);
static spinlock_t consoles_lock = SPIN_LOCK_UNLOCKED;

static int read_io_int_handler(struct device *dev, struct io_op *ioop, struct irb *irb)
{
	struct console_line *cline;
	struct ccw *ccw;
	void *ptr;

	/* Device End is set, we're done */
	if (!(irb->scsw.dev_status & 0x04)) {
		ioop->err = -EAGAIN;
		return 0;
	}

	ccw = (struct ccw*) (u64) ioop->orb.addr;
	ptr = (void*) (u64) ccw->addr;
	cline = container_of(ptr, struct console_line, buf);

	cline->len = strnlen((char*) cline->buf, CON_MAX_LINE_LEN-1);
	cline->state = CON_STATE_IO;

	ioop->err = 0;
	return 0;
}

static void do_issue_read(struct console *con, struct io_op *ioop, struct ccw *ccws)
{
	struct console_line *cline;
	int found;

	found = 0;

	atomic_dec(&con->dev->attention);

	spin_lock(&con->lock);
	list_for_each_entry(cline, &con->read_lines, lines) {
		if (cline->state != CON_STATE_FREE)
			continue;

		cline->state = CON_STATE_PENDING;
		found = 1;
		break;
	}

	if (!found) {
		/*
		 * No unused console lines, time to allocate a new one
		 */
		cline = malloc(CON_LINE_ALLOC_SIZE, ZONE_NORMAL);
		BUG_ON(!cline);

		cline->state = CON_STATE_PENDING;
		list_add_tail(&cline->lines, &con->read_lines);
	}
	spin_unlock(&con->lock);

	/* clear the buffer to allow strlen on the result */
	memset(cline->buf, 0, CON_MAX_LINE_LEN);

	memset(ccws, 0, sizeof(struct ccw));

	ccws[0].addr  = ADDR31(cline->buf);
	ccws[0].count = CON_MAX_LINE_LEN - 1;
	ccws[0].cmd   = 0x0a;
	ccws[0].flags = CCW_FLAG_SLI;

	memset(&ioop->orb, 0, sizeof(struct orb));
	ioop->orb.lpm  = 0xff;
	ioop->orb.addr = ADDR31(ccws);
	ioop->orb.f    = 1;

	ioop->handler = read_io_int_handler;
	ioop->dtor = NULL;

	submit_io(con->dev, ioop, CAN_SLEEP);
}

/**
 * console_flusher - iterates over a console's buffers and initiates the IO
 */
static int console_flusher(void *data)
{
	struct console *con = data;
	struct console_line *cline;
	int free_count;
	int ccw_count;

	/* needed for the IO */
	struct io_op ioop;
	struct ccw ccws[CON_MAX_FLUSH_LINES];

	for(;;) {
		if (atomic_read(&con->dev->attention))
			do_issue_read(con, &ioop, ccws);

		spin_lock(&con->lock);

		/*
		 * free all the lines we just finished the IO for
		 */
		free_count = 0;
		list_for_each_entry(cline, &con->write_lines, lines) {
			if (cline->state != CON_STATE_IO)
				continue;

			cline->state = CON_STATE_FREE;
			free_count++;

			if (free_count > CON_MAX_FREE_LINES) {
				list_del(&cline->lines);
				free(cline);
			}
		}

		/*
		 * find at most CON_MAX_FLUSH_LINES of CON_STATE_PENDING
		 * lines and shove necessary information into the right
		 * CCW
		 */
		ccw_count = 0;
		list_for_each_entry(cline, &con->write_lines, lines) {
			if (ccw_count >= CON_MAX_FLUSH_LINES)
				break;

			if (cline->state != CON_STATE_PENDING)
				continue;

			cline->state = CON_STATE_IO;

			ccws[ccw_count].addr = ADDR31(cline->buf);
			ccws[ccw_count].count = cline->len;
			ccws[ccw_count].cmd   = 0x01; /* write */
			ccws[ccw_count].flags = CCW_FLAG_CC | CCW_FLAG_SLI;

			ccw_count++;
		}

		/*
		 * We don't need the lock anymore
		 */
		spin_unlock(&con->lock);

		/*
		 * Anything to do?
		 */
		if (!ccw_count) {
			schedule();
			continue;
		}

		/*
		 * Clear Command-Chaining on the last CCW
		 */
		ccws[ccw_count-1].flags &= ~CCW_FLAG_CC;

		/*
		 * Now, set up the ORB and CCW
		 */
		memset(&ioop.orb, 0, sizeof(struct orb));
		ioop.orb.lpm = 0xff;
		ioop.orb.addr = ADDR31(ccws);
		ioop.orb.f = 1;		/* format 1 CCW */

		/*
		 * Set up the operation handler pointers, and start the IO
		 */
		ioop.handler = NULL;
		ioop.dtor = NULL;

		submit_io(con->dev, &ioop, CAN_SLEEP);
	}

	return 0;
}

/**
 * register_console - generic device registration callback
 * @dev:	console device to register
 */
static int register_console(struct device *dev)
{
	struct console *con;
	struct console_line *cline;

	dev_get(dev);

	con = malloc(sizeof(struct console), ZONE_NORMAL);
	BUG_ON(!con);

	con->sys    = NULL;
	con->dev    = dev;
	con->lock   = SPIN_LOCK_UNLOCKED;
	INIT_LIST_HEAD(&con->write_lines);
	INIT_LIST_HEAD(&con->read_lines);

	/*
	 * alloc one read-line
	 */
	cline = malloc(CON_LINE_ALLOC_SIZE, ZONE_NORMAL);
	BUG_ON(!cline);
	cline->state = CON_STATE_FREE;
	list_add(&cline->lines, &con->read_lines);

	spin_lock(&consoles_lock);
	list_add_tail(&con->consoles, &consoles);
	spin_unlock(&consoles_lock);

	return 0;
}

static void print_splash(struct console *con)
{
	int i;

	for(i = 0; splash[i]; i++)
		con_printf(con, splash[i]);

	con_printf(con, "HVF VERSION " VERSION "\n\n");
}

struct console* start_oper_console(void)
{
	struct device *dev;

	/*
	 * We only start the operator console
	 */

	dev = find_device_by_ccuu(OPER_CONSOLE_CCUU);
	BUG_ON(IS_ERR(dev));

	return console_enable(dev);
}

void* console_enable(struct device *dev)
{
	char name[TASK_NAME_LEN+1];
	struct console *con;

	con = find_console(dev);
	if (!IS_ERR(con))
		return con;

	if (register_console(dev))
		return ERR_PTR(-ENOMEM);

	/* try again, this time, we should always find it! */
	con = find_console(dev);
	if (IS_ERR(con))
		return con;

	atomic_inc(&con->dev->in_use);

	snprintf(name, TASK_NAME_LEN, "%05X-conflsh", con->dev->sch);

	create_task(name, console_flusher, con);

	print_splash(con);

	return con;
}

int con_read_pending(struct console *con)
{
	struct console_line *cline;
	int ret = 0;

	spin_lock(&con->lock);

	list_for_each_entry(cline, &con->read_lines, lines) {
		if (cline->state == CON_STATE_IO) {
			ret = 1;
			break;
		}
	}

	spin_unlock(&con->lock);

	return ret;
}

int con_read(struct console *con, u8 *buf, int size)
{
	struct console_line *cline;
	int len;

	spin_lock(&con->lock);

	list_for_each_entry(cline, &con->read_lines, lines) {
		if (cline->state == CON_STATE_IO)
			goto found;
	}

	spin_unlock(&con->lock);

	return -1;

found:
	len = (size-1 < cline->len) ? size-1 : cline->len;

	memcpy(buf, cline->buf, len);
	buf[len] = '\0';

	cline->state = CON_STATE_FREE;

	spin_unlock(&con->lock);

	return len;
}

int con_write(struct console *con, u8 *buf, int len)
{
	int bytes = 0;
	struct console_line *cline;

	spin_lock(&con->lock);

	list_for_each_entry(cline, &con->write_lines, lines) {
		if (cline->state == CON_STATE_FREE)
			goto found;
	}

	/* None found, can we allocate a new one? */
	cline = malloc(CON_LINE_ALLOC_SIZE, ZONE_NORMAL);
	if (!cline)
		goto abort;

	list_add_tail(&cline->lines, &con->write_lines);

found:
	cline->state = CON_STATE_PENDING;

	cline->len = (len < CON_MAX_LINE_LEN) ?  len : CON_MAX_LINE_LEN;
	memcpy(cline->buf, buf, cline->len);

	/*
	 * All done here. The async thread will pick up the line of text,
	 * and issue the IO.
	 */

abort:
	spin_unlock(&con->lock);

	return bytes;
}

void for_each_console(void (*f)(struct console *con))
{
	struct console *con;

	if (!f)
		return;

	spin_lock(&consoles_lock);
	list_for_each_entry(con, &consoles, consoles)
		f(con);
	spin_unlock(&consoles_lock);
}

struct console* find_console(struct device *dev)
{
	struct console *con;

	if (!dev)
		return ERR_PTR(-ENOENT);

	spin_lock(&consoles_lock);
	list_for_each_entry(con, &consoles, consoles) {
		if (con->dev == dev)
			goto found;
	}
	con = ERR_PTR(-ENOENT);
found:
	spin_unlock(&consoles_lock);
	return con;
}
