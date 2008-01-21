#include <console.h>
#include <slab.h>
#include <sched.h>
#include <directory.h>

/*
 * List of all consoles on the system
 */
static LIST_HEAD(consoles);
static spinlock_t consoles_lock = SPIN_LOCK_UNLOCKED;

/**
 * console_flusher - iterates over a console's buffers and initiates the IO
 */
static int console_flusher(void *data)
{
	struct console *con = data;
	int idx;
	int midaw_count;
	int len;

	/* needed for the IO */
	struct io_op ioop;
	struct ccw ccw;
	struct midaw midaws[CON_MAX_FLUSH_LINES];

	/*
	 * Workaround to make entering the loop below simpler
	 */
	atomic_set(&ioop.done, 1);
	
	for(;;) {
		/*
		 * A do-while to force a call schedule() at least once.
		 */
		do {
			schedule();
		} while(!atomic_read(&ioop.done));

		spin_lock(&con->lock);

		/*
		 * free all the lines we just finished the IO for
		 */
		for(idx=0; idx < con->nlines; idx++) {
			if (con->lines[idx]->state != CON_STATE_IO)
				continue;

			con->lines[idx]->state = CON_STATE_FREE;
		}

		/*
		 * find at most CON_MAX_FLUSH_LINES of CON_STATE_PENDING
		 * lines _OR_ at most 150 bytes to send, mark them as IO,
		 * and shove necessary information into the right MIDAW
		 *
		 * FIXME: If a single line contains more than 150
		 * characters, it will cause all lines from that point on to
		 * never appear on the console. Perhaps the best fix would
		 * be to prevent the addition of the buffer as a line in
		 * con_write().
		 */
		for(idx=0, midaw_count=0, len = 0;
		    (idx < con->nlines) && (midaw_count < CON_MAX_FLUSH_LINES);
		    idx++) {
			if (con->lines[idx]->state != CON_STATE_PENDING)
				continue;

			if ((len + con->lines[idx]->len) > 150)
				break;

			con->lines[idx]->state = CON_STATE_IO;

			memset(&midaws[midaw_count], 0, sizeof(struct midaw));
			midaws[midaw_count].count = con->lines[idx]->len;
			midaws[midaw_count].addr  = (u64) con->lines[idx]->buf;
			len += con->lines[idx]->len;

			midaw_count++;
		}

		/*
		 * We don't need the lock anymore
		 */
		spin_unlock(&con->lock);

		/*
		 * Anything to do?
		 */
		if (!midaw_count)
			continue;

		/*
		 * Mark the last MIDAW as the last one
		 */
		midaws[midaw_count-1].last = 1;

		/*
		 * Now, set up the ORB and CCW
		 */
		memset(&ioop.orb, 0, sizeof(struct orb));
		ioop.orb.lpm = 0xff;
		ioop.orb.addr = (u32) (u64) &ccw; // FIXME: make sure ccw is in <2GB
		ioop.orb.f = 1;		/* format 1 CCW */
		ioop.orb.d = 1;		/* MIDAW CCW */

		memset(&ccw, 0, sizeof(struct ccw));
		ccw.cmd   = 0x01; /* write */
		ccw.sli   = 1;
		ccw.mida  = 1;
		ccw.count = len;
		ccw.addr  = (u32) (u64) midaws; // FIXME: make sure midaws is in <2GB

		/*
		 * Set up the operation handler pointers, and start the IO
		 */
		ioop.ssid = con->dev->sch;
		ioop.handler = NULL;
		ioop.dtor = NULL;

		while(submit_io(&ioop, CAN_SLEEP))
			schedule();
	}
}

/**
 * register_console - generic device registration callback
 * @dev:	console device to register
 */
int register_console(struct device *dev)
{
	struct console *con;

	con = malloc(sizeof(struct console), ZONE_NORMAL);
	BUG_ON(!con);

	con->dev    = dev;
	con->lock   = SPIN_LOCK_UNLOCKED;
	con->nlines = 0;

	spin_lock(&consoles_lock);
	list_add_tail(&con->consoles, &consoles);
	spin_unlock(&consoles_lock);

	return 0;
}

void start_consoles()
{
	struct user *u;

	/*
	 * For now, we only start the operator console
	 */

	u = find_user_by_id("operator");
	BUG_ON(IS_ERR(u));

	BUG_ON(list_empty(&consoles));
	u->con = list_first_entry(&consoles, struct console, consoles);

	create_task(console_flusher, u->con);
}

int oper_con_write(u8 *buf, int len)
{
	struct user *u;

	u = find_user_by_id("operator");
	BUG_ON(IS_ERR(u));

	return con_write(u->con, buf, len);
}

int con_write(struct console *con, u8 *buf, int len)
{
	int idx;
	int bytes = 0;

	spin_lock(&con->lock);

	for(idx=0; idx<con->nlines; idx++) {
		if (con->lines[idx]->state == CON_STATE_FREE)
			goto found;
	}

	/* None found, can we allocate a new one? */
	if (con->nlines == CON_MAX_LINES)
		/*
		 * FIXME: maybe this is where we should use a mutex instead
		 * of a spinlock; this would allow us to grab it if
		 * possible, and fail only if we'd have to sleep in an
		 * non-schedulable case
		 */
		goto abort;

	/* allocate a new one */
	idx = con->nlines;

	con->lines[idx] = malloc(CON_LINE_ALLOC_SIZE, ZONE_NORMAL);
	if (!con->lines[idx])
		goto abort;

	con->nlines++;

found:
	con->lines[idx]->state = CON_STATE_PENDING;

	con->lines[idx]->len = (len < CON_MAX_LINE_LEN) ?
					len : CON_MAX_LINE_LEN;
	memcpy(con->lines[idx]->buf, buf, con->lines[idx]->len);

	/*
	 * All done here. The async thread will pick up the line of text,
	 * and issue the IO.
	 */

abort:
	spin_unlock(&con->lock);

	return bytes;
}
