#ifndef __SCHED_H
#define __SCHED_H

#include <list.h>
#include <page.h>

#define SCHED_CANSLEEP		1	/* safe to sleep */

#define TASK_RUNNING		0
#define TASK_SLEEPING		1

#define STACK_FRAME_SIZE	160

/*
 * The timer hardware subtracts 1 from the 51st bit every microsecond
 */
#define CLK_MICROSEC		0x1000UL

#define HZ			100	/* number of ticks per second */

struct psw {
	u8 _zero0:1,
	   r:1,			/* PER Mask (R)			*/
	   _zero1:3,
	   t:1,			/* DAT Mode (T)			*/
	   io:1,		/* I/O Mask (IO)		*/
	   ex:1;		/* External Mask (EX)		*/

	u8 key:4,		/* Key				*/
	   _zero2:1,
	   m:1,			/* Machine-Check Mask (M)	*/
	   w:1,			/* Wait State (W)		*/
	   p:1;			/* Problem State (P)		*/
		
	u8 as:2,		/* Address-Space Control (AS)	*/
	   cc:2,		/* Condition Code (CC)		*/
	   prog_mask:4;		/* Program Mask			*/

	u8 _zero3:7,	
	   ea:1;		/* Extended Addressing (EA)	*/

	u32 ba:1,		/* Basic Addressing (BA)	*/
	    _zero4:31;

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
	struct list_head proc_list;	/* processes list */

	int state;			/* state */
};

extern void init_sched();		/* initialize the scheduler */
extern int create_task(int (*f)());	/* create a new task */
extern void schedule();

/**
 * current - the current task's task struct
 */
#define current		extract_task()

/**
 * extract_task_ptr - given an address anywhere on the stack, return the
 * associated task struct
 * @stack:	a pointer to anywhere in the stack
 */
static inline struct task *extract_task_ptr(void *stack)
{
	u8 *ptr;
	
	ptr = (u8*) (((u64) stack) & ~(PAGE_SIZE-1));
	ptr += PAGE_SIZE - sizeof(void*);

	return *((struct task **) ptr);
}

/**
 * extract_task - return the associated task struct
 */
static inline struct task *extract_task()
{
	int unused;

	return extract_task_ptr(&unused);
}

/**
 * set_task_ptr - set the stack's task struct pointer
 * @stack:	a pointer to anywhere in the stack
 * @task:	task struct pointer to be associated with the stack
 */
static inline void set_task_ptr(void *stack, struct task *task)
{
	u8 *ptr;
	
	ptr = (u8*) (((u64) stack) & ~(PAGE_SIZE-1));
	ptr += PAGE_SIZE - sizeof(void*);

	*((struct task**)ptr) = task;
}

#endif
