#ifndef __IO_H
#define __IO_H

#include <channel.h>
#include <atomic.h>
#include <list.h>

struct device;

/*
 * Defines an I/O operation
 *
 * This structure contains any and all state one should need to service any
 * I/O interruption.
 */
struct io_op {
	struct orb orb;		/* Operation Request Block */

	struct list_head list;	/* list of in-flight operations */

	int (*handler)(struct device *dev, struct io_op *ioop, struct irb *irb);
				/* I/O specific callback */
	void (*dtor)(struct device *dev, struct io_op *ioop);
				/* I/O specific destructor */

	int err;		/* return code */
	atomic_t done;		/* has the operation completed */
};

extern void init_io(void);
extern int submit_io(struct device *dev, struct io_op *oop, int flags);

#endif
