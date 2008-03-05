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
	struct console_line *cline;
	int free_count;
	int ccw_count;

	/* needed for the IO */
	struct io_op ioop;
	struct ccw ccws[CON_MAX_FLUSH_LINES];

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

			// FIXME: make sure buffer is in <2GB
			ccws[ccw_count].addr = (u32) (u64) cline->buf;
			ccws[ccw_count].count = cline->len;
			ccws[ccw_count].cmd   = 0x01; /* write */
			ccws[ccw_count].sli   = 1;
			ccws[ccw_count].cc    = 1; /* command chaining */

			ccw_count++;
		}

		/*
		 * We don't need the lock anymore
		 */
		spin_unlock(&con->lock);

		/*
		 * Anything to do?
		 */
		if (!ccw_count)
			continue;

		/*
		 * Clear Command-Chaining on the last CCW
		 */
		ccws[ccw_count-1].cc = 0;

		/*
		 * Now, set up the ORB and CCW
		 */
		memset(&ioop.orb, 0, sizeof(struct orb));
		ioop.orb.lpm = 0xff;
		ioop.orb.addr = (u32) (u64) ccws; // FIXME: make sure ccw is in <2GB
		ioop.orb.f = 1;		/* format 1 CCW */

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
	INIT_LIST_HEAD(&con->write_lines);

	spin_lock(&consoles_lock);
	list_add_tail(&con->consoles, &consoles);
	spin_unlock(&consoles_lock);

	return 0;
}

/**
 * console_interrupt - generic console callback
 */
int console_interrupt()
{
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
