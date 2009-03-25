#ifndef __SCHED_H
#define __SCHED_H

#include <list.h>
#include <page.h>
#include <dat.h>
#include <clock.h>

#define CAN_SLEEP		1	/* safe to sleep */

#define TASK_RUNNING		0
#define TASK_SLEEPING		1

#define STACK_FRAME_SIZE	160

#define SCHED_SLICE_MS		30	/* 30ms scheduler slice */
#define SCHED_TICKS_PER_SLICE	(HZ / SCHED_SLICE_MS)

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

/*
 * saved registers for guests
 *
 * NOTE: some registers are saved in the SIE control block!
 */
struct guest_regs {
	u64 gpr[16];
	u32 ar[16];
	u64 fpr[64];
	u32 fpcr;
};

/* saved registers for CP tasks */
struct regs {
	struct psw psw;
	u64 gpr[16];
	u64 cr1;
};

/*
 * These states mirror those described in chapter 4 of SA22-7832-06
 */
enum virt_cpustate {
	GUEST_STOPPED = 0,
	GUEST_OPERATING,
	GUEST_LOAD,
	GUEST_CHECKSTOP,
};

#include <sie.h>

struct virt_cpu {
	/* the SIE control block is picky about alignment */
	struct sie_cb sie_cb;

	struct guest_regs regs;

	enum virt_cpustate state;
};

#define TASK_NAME_LEN		16

/*
 * This structure describes a running process.
 */
struct task {
	struct regs regs;		/* saved registers */

	struct list_head run_queue;	/* runnable list */
	struct list_head proc_list;	/* processes list */

	u64 slice_end_time;		/* end of slice time (ticks) */

	struct virt_cpu *cpu;		/* guest cpu */

	int state;			/* state */

	char name[TASK_NAME_LEN+1];	/* task name */
};

struct virt_sys {
	struct task *task;		/* the virtual CPU task */
	struct user *directory;		/* the directory information */

	struct console *con;		/* the login console */

	struct list_head guest_pages;	/* list of guest pages */

	struct address_space as;	/* the guest storage */
};

extern void init_sched(void);		/* initialize the scheduler */
extern struct task* create_task(char *name, int (*f)(void*), void*);
					/* create a new task */
extern void schedule(void);		/* yield the cpu */
extern void __schedule(struct psw *);	/* scheduler helper - use with caution */
extern void __schedule_svc(void);

extern void list_tasks(struct console *con,
		       void (*f)(struct console *, struct task*));

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

#endif
