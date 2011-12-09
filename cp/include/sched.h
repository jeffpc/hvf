/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __SCHED_H
#define __SCHED_H

#include <list.h>
#include <page.h>
#include <dat.h>
#include <clock.h>
#include <interrupt.h>
#include <ldep.h>

#define CAN_SLEEP		1	/* safe to sleep */
#define CAN_LOOP		2	/* safe to busy-wait */

#define TASK_RUNNING		0
#define TASK_SLEEPING		1
#define TASK_LOCKED		2
#define TASK_ZOMBIE		3

#define STACK_FRAME_SIZE	160

#define SCHED_SLICE_MS		30	/* 30ms scheduler slice */
#define SCHED_TICKS_PER_SLICE	(HZ / SCHED_SLICE_MS)

#define HZ			100	/* number of ticks per second */

/* saved registers for CP tasks */
struct regs {
	struct psw psw;
	u64 gpr[16];
	u64 cr1;
};

#define TASK_NAME_LEN		16

/*
 * This structure describes a running process.
 */
struct task {
	struct regs regs;		/* saved registers */

	struct list_head run_queue;	/* runnable list */
	struct list_head proc_list;	/* processes list */
	struct list_head blocked_list;	/* blocked on mutex/etc. list */

	u64 slice_end_time;		/* end of slice time (ticks) */

	int state;			/* state */

	void *stack;			/* the stack */

	char name[TASK_NAME_LEN+1];	/* task name */

	/* lock dependency tracking */
	int nr_locks;
	struct held_lock lock_stack[LDEP_STACK_SIZE];
};

extern void init_sched(void);		/* initialize the scheduler */
extern struct task* create_task(char *name, int (*f)(void*), void*);
					/* create a new task */
extern void __schedule(struct psw *,
		       int newstate);	/* scheduler helper - use with caution */
extern void __schedule_svc(void);
extern void __schedule_blocked_svc(void);
extern void __schedule_exit_svc(void);

extern void make_runnable(struct task *task);

extern void list_tasks(void (*f)(struct task*, void*),
                       void *priv);

/**
 * current - the current task's task struct
 */
#define current		extract_task()

#define PSA_CURRENT	((struct task**) 0x290)

/**
 * extract_task - return the current task struct
 */
static inline struct task *extract_task(void)
{
	return *PSA_CURRENT;
}

/**
 * set_task_ptr - set the stack's task struct pointer
 * @task:	task struct pointer to be made current
 */
static inline void set_task_ptr(struct task *task)
{
	*PSA_CURRENT = task;
}

/**
 * schedule - used to explicitly yield the cpu
 */
static inline void schedule(void)
{
	/*
	 * if we are not interruptable, we shouldn't call any functions that
	 * may sleep - schedule() is guaranteed to sleep :)
	 */
	BUG_ON(!interruptable());

	asm volatile(
		"	svc	%0\n"
	: /* output */
	: /* input */
	  "i" (SVC_SCHEDULE)
	);
}

/**
 * schedule_blocked - used to explicitly yield the cpu without readding the
 * task to the runnable queue
 */
static inline void schedule_blocked(void)
{
	/*
	 * if we are not interruptable, we shouldn't call any functions that
	 * may sleep - schedule() is guaranteed to sleep :)
	 */
	BUG_ON(!interruptable());

	asm volatile(
		"	svc	%0\n"
	: /* output */
	: /* input */
	  "i" (SVC_SCHEDULE_BLOCKED)
	);
}

/**
 * exit - schedule with the intent to terminate the current task
 */
static inline void exit(void)
{
	/*
	 * if we are not interruptable, we shouldn't call any functions that
	 * may sleep - schedule() is guaranteed to sleep :)
	 */
	BUG_ON(!interruptable());

	asm volatile(
		"	svc	%0\n"
	: /* output */
	: /* input */
	  "i" (SVC_SCHEDULE_EXIT)
	);
}

#endif
