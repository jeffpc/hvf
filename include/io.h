#ifndef __IO_H
#define __IO_H

#include <channel.h>
#include <atomic.h>

/*
 * Defines an I/O operation
 *
 * This structure contains any and all state one should need to service any
 * I/O interruption.
 */
struct io_op {
	struct orb orb;		/* Operation Request Block */

	int (*handler)(struct io_op *ioop, struct irb *irb);
				/* I/O specific callback */
	void (*dtor)(struct io_op *ioop);
				/* I/O specific destructor */

	int ssid;		/* subsystem ID */

	int err;		/* return code */
	atomic_t done;		/* has the operation completed */
};

/*
 * This is an ugly hack to avoid having to lock the entire in-flight ops
 * array; instead, we scan the list linearly, until we find an unused
 * io_op_inflight_entry.
 */
struct io_op_inflight_entry {
	struct io_op *op;
	atomic_t used;
};

#define MAX_IOS		128	/* max number of in-flight IO ops */
#define IO_PARAM_BASE	0x10000000

extern void init_io();
extern int submit_io(struct io_op *oop, int flags);

#endif
