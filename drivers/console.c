#include <console.h>
#include <slab.h>

int con_write(struct console *con, u8 *buf, int len)
{
	int idx;
	int bytes = 0;

	spin_lock(&con->lock);

	for(idx=0; idx<con->nlines; idx++) {
		if (!con->lines[idx]->used)
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
	con->lines[idx]->used = 1;

	con->lines[idx]->len = (len < CON_MAX_LINE_LEN) ?
					len : CON_MAX_LINE_LEN;
	memcpy(con->lines[idx]->buf, buf, con->lines[idx]->len);

	/*
	 * FIXME:
	 *   - something needs to issue the IO
	 */

abort:
	spin_unlock(&con->lock);

	return bytes;
}
