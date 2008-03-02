#ifndef __IO_H
#define __IO_H

#include <channel.h>
#include <atomic.h>
#include <list.h>

/*
 * Defines an I/O operation
 *
 * This structure contains any and all state one should need to service any
 * I/O interruption.
 */
struct io_op {
	struct orb orb;		/* Operation Request Block */

	struct list_head list;	/* list of in-flight operations */

	int (*handler)(struct io_op *ioop, struct irb *irb);
				/* I/O specific callback */
	void (*dtor)(struct io_op *ioop);
				/* I/O specific destructor */

	int ssid;		/* subsystem ID */

	int err;		/* return code */
	atomic_t done;		/* has the operation completed */
};

#define MAX_IOS		128	/* max number of in-flight IO ops */

extern void init_io();
extern int submit_io(struct io_op *oop, int flags);

#endif
