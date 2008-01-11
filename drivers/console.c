#include <console.h>
#include <slab.h>
#include <sched.h>

static struct console console;

/**
 * console_flusher - iterates over a console's buffers and initiates the IO
 */
static int console_flusher()
{
	struct console *con = &console;
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
		while(!atomic_read(&ioop.done))
			schedule();

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
		 * lines, mark them as IO, and shove necessary information
		 * into the right MIDAW
		 */
		for(idx=0, midaw_count=0, len = 0;
		    (idx < con->nlines) && (midaw_count < CON_MAX_FLUSH_LINES);
		    idx++) {
			if (con->lines[idx]->state != CON_STATE_PENDING)
				continue;

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

void init_oper_console()
{
	console.dev	= NULL;
	console.lock	= SPIN_LOCK_UNLOCKED;
	console.nlines	= 0;
}

void start_oper_console()
{
	console.dev = find_device_by_type(0x3215, 0);
	BUG_ON(IS_ERR(console.dev));

	create_task(console_flusher);
}

int oper_con_write(u8 *buf, int len)
{
	return con_write(&console, buf, len);
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

	con->lines[idx] = malloc(CON_LINE_ALLOC_SIZE);
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
