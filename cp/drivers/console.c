/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <console.h>
#include <slab.h>
#include <sched.h>
#include <directory.h>
#include <vsprintf.h>
#include <spool.h>
#include <buddy.h>

/*
 * List of all consoles on the system
 */
static LIST_HEAD(consoles);
static spinlock_t consoles_lock = SPIN_LOCK_UNLOCKED;

static void do_issue_read(struct console *con, struct io_op *ioop, struct ccw *ccws)
{
	int ret;

	atomic_dec(&con->dev->attention);

	/* clear the buffer to allow strlen on the result */
	memset(con->bigbuf, 0, CONSOLE_LINE_LEN+1);

	ccws[0].addr  = ADDR31(con->bigbuf);
	ccws[0].count = CONSOLE_LINE_LEN;
	ccws[0].cmd   = 0x0a;
	ccws[0].flags = CCW_FLAG_SLI;

	memset(&ioop->orb, 0, sizeof(struct orb));
	ioop->orb.lpm  = 0xff;
	ioop->orb.addr = ADDR31(ccws);
	ioop->orb.f    = 1;

	ioop->handler = NULL;
	ioop->dtor = NULL;

	submit_io(con->dev, ioop, CAN_SLEEP);

	ret = spool_append_rec(con->rlines, con->bigbuf,
			       strnlen(con->bigbuf, CONSOLE_LINE_LEN));
	BUG_ON(ret);
}

/**
 * console_flusher - iterates over a console's buffers and initiates the IO
 */
static int console_flusher(void *data)
{
	struct console *con = data;
	u8 *buf;
	u16 len;
	u16 left;

	/* needed for the IO */
	struct io_op ioop;
	struct ccw ccws[PAGE_SIZE/CONSOLE_LINE_LEN];
	int i;

	for(;;) {
		/* FIXME: this should be a sub-unless-zero */
		if (atomic_read(&con->dev->attention))
			do_issue_read(con, &ioop, ccws);

		buf = con->bigbuf;
		left = PAGE_SIZE;

		for(i=0; left >= CONSOLE_LINE_LEN; i++) {
			len = CONSOLE_LINE_LEN;
			if (spool_grab_rec(con->wlines, buf, &len))
				break;

			ccws[i].addr  = ADDR31(buf);
			ccws[i].count = len;
			ccws[i].cmd   = 0x01; /* write */
			ccws[i].flags = CCW_FLAG_CC | CCW_FLAG_SLI;

			left -= len;
			buf += len;
		}

		if (!i) {
			schedule();
			continue;
		}

		ccws[i-1].flags &= ~CCW_FLAG_CC;

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
	struct page *page;
	int ret = 0;

	dev_get(dev);

	con = malloc(sizeof(struct console), ZONE_NORMAL);
	BUG_ON(!con);

	con->sys    = NULL;
	con->dev    = dev;

	con->wlines = alloc_spool();
	if (IS_ERR(con->wlines)) {
		ret = PTR_ERR(con->wlines);
		goto out;
	}

	con->rlines = alloc_spool();
	if (IS_ERR(con->rlines)) {
		ret = PTR_ERR(con->rlines);
		goto out_free;
	}

	page = alloc_pages(0, ZONE_NORMAL);
	if (!page) {
		ret = -ENOMEM;
		goto out_free2;
	}

	con->bigbuf = page_to_addr(page);

	spin_lock(&consoles_lock);
	list_add_tail(&con->consoles, &consoles);
	spin_unlock(&consoles_lock);

out:
	return ret;

out_free2:
	free_spool(con->rlines);
out_free:
	free_spool(con->wlines);
	goto out;
}

static void print_splash(struct console *con)
{
	struct logo_rec *rec;
	struct logo *logo;

	list_for_each_entry(logo, &sysconf.logos, list) {
		if (con->dev->type != logo->devtype)
			continue;

		/* FIXME: check the connection type */

		list_for_each_entry(rec, &logo->lines, list)
			con_printf(con, "%*.*s\n", CONFIG_LRECL,
				   CONFIG_LRECL, (char*) rec->data);
		break;
	}

	con_printf(con, "HVF VERSION " VERSION "\n\n");
}

struct console* start_oper_console(void)
{
	struct device *dev;

	/*
	 * We only start the operator console
	 */

	dev = find_device_by_ccuu(sysconf.oper_con);
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
	return spool_nrecs(con->rlines);
}

int con_read(struct console *con, u8 *buf, int size)
{
	u16 len = size;

	if (spool_grab_rec(con->rlines, buf, &len))
		return -1;

	return len;
}

int con_write(struct console *con, u8 *buf, int len)
{
	if (spool_append_rec(con->wlines, buf, len))
		return 0;

	return len;
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
