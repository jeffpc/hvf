#ifndef __SCHED_H
#define __SCHED_H

#include <list.h>

#define SCHED_CANSLEEP		1	/* safe to sleep */

#define TASK_UNUSED		0
#define TASK_RUNNING		1
#define TASK_SLEEPING		2

#define TASK_NUCLEUS		0
#define TASK_USER		1

struct psw {
	u8 bits[8];
	u64 ptr;
};

struct regs {
	struct psw psw;
	u64 gpr[16];
	u64 cr[16];
	u32 ar[16];
	/* FIXME: fpr[16] */
};

/*
 * This structure describes a running process.
 */
struct task {
	struct regs regs;		/* saved registers */

	struct list_head run_queue;	/* runnable list */
	struct list_head proc_queue;	/* processes list */

	int status;			/* status */

	int type;			/* task type */
};

extern void init_sched();		/* initialize the scheduler */

#endif
