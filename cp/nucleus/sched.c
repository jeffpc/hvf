/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <sched.h>
#include <page.h>
#include <buddy.h>
#include <slab.h>
#include <magic.h>

/* list of all runnable processes */
static struct list_head runnable;

/* list of all processes in the system */
static struct list_head processes;

/* idle task */
static struct task idle_task;

/**
 * start_task - helper used to start the task's code
 * @f:	function to execute
 */
static void start_task(int (*f)(void*), void *data)
{
	/*
	 * Start executing the code
	 */
	if (f)
		(*f)(data);

	ldep_no_locks();

	exit();

	/* unreachable */
	BUG();
}

/**
 * __init_task - helper to initialize the task structure
 * @task:	task struct to initialize
 * @f:		pointer to function to execute
 * @stack:	pointer to the page for stack
 */
static void __init_task(struct task *task, void *f, void *data, void *stack)
{
	memset(task, 0, sizeof(struct task));

	task->regs.psw.io	= 1;
	task->regs.psw.ex	= 1;
	task->regs.psw.ea	= 1;
	task->regs.psw.ba	= 1;

	/* code to execute */
	task->regs.psw.ptr = (u64) start_task;

	task->regs.gpr[2]  = (u64) f;
	task->regs.gpr[3]  = (u64) data;
	task->regs.gpr[15] = ((u64) stack) + PAGE_SIZE - STACK_FRAME_SIZE;

	task->state = TASK_SLEEPING;

	task->stack = stack;

	task->nr_locks = 0;
}

/**
 * init_idle_task - initialize the idle task
 *
 * Since the initial thread of execution already has a stack, and the task
 * struct is a global variable, all that is left for us to do is to
 * associate the current stack with idle_task.
 */
static void init_idle_task(void)
{
	u64 stack;

	stack = ((u64) &stack) & ~(PAGE_SIZE-1);

	__init_task(&idle_task, NULL, NULL, (void*) stack);
	set_task_ptr(&idle_task);

	/*
	 * NOTE: The idle task is _not_ supposed to be on either of the
	 * two process lists.
	 */
}

/**
 * create_task - create a new task
 * @name:	name for the task
 * @f:		function pointer to where thread of execution should begin
 * @data:	arbitrary data to pass to the task
 */
struct task* create_task(char *name, int (*f)(void *), void *data)
{
	struct page *page;
	struct task *task;

	/*
	 * Allocate a page for stack, and struct task itself
	 */
	page = alloc_pages(0, ZONE_NORMAL);
	task = malloc(sizeof(struct task), ZONE_NORMAL);
	if (!page || !task)
		goto out;

	/*
	 * Set up task's state
	 */
	__init_task(task, f, data, page_to_addr(page));

	strncpy(task->name, name, TASK_NAME_LEN);

	/*
	 * Add the task to the scheduler lists
	 */
	list_add_tail(&task->proc_list, &processes);
	list_add_tail(&task->run_queue, &runnable);

	return task;

out:
	free(task);
	free_pages(page_to_addr(page), 0);

	return ERR_PTR(-ENOMEM);
}

/**
 * __sched - core of the scheduler code. This decides which task to run
 * next, and switches over to it
 */
static void __sched(int force_idle)
{
	struct task *next;

	/*
	 * If there are no runnable tasks, let's use the idle thread
	 */
	if (force_idle || list_empty(&runnable)) {
		next = &idle_task;
		goto run;
	}

	next = list_first_entry(&runnable, struct task, run_queue);
	BUG_ON(!next);

	/*
	 * Remove the new task from the run queue & mark it as running
	 */
	list_del(&next->run_queue);
	next->state = TASK_RUNNING;

run:
	next->slice_end_time = (force_idle) ? force_idle : ticks + SCHED_TICKS_PER_SLICE;

	/*
	 * NOTE: Because we need to load the registers _BEFORE_ we issue
	 * lpswe, we have to place the new psw into PSA and use register 0
	 */
	memcpy(PSA_TMP_PSW, &next->regs.psw, sizeof(struct psw));
	set_task_ptr(next);

	load_pasce(next->regs.cr1);

	/*
	 * Load the next task
	 *
	 * FIXME: fp? ar? cr?
	 */
	asm volatile(
		"lmg	%%r0,%%r15,%0\n"	/* load gpr */
		"lpswe	%1\n"			/* load new psw */
	: /* output */
	: /* input */
	  "m" (next->regs.gpr[0]),
	  "m" (*PSA_TMP_PSW)
	);

	/* unreachable */
	BUG();
}

/**
 * schedule_preempted - called to switch tasks
 */
void __schedule(struct psw *old_psw, int newstate)
{
	struct task *prev;
	int force_idle = 0;

	/*
	 * Save last process's state
	 */

	prev = current;
	BUG_ON(!prev);

	/*
	 * Save the registers (gpr, psw, ...)
	 *
	 * FIXME: fp? ar? cr?
	 */
	memcpy(prev->regs.gpr, PSA_INT_GPR, sizeof(u64)*16);
	memcpy(&prev->regs.psw, old_psw, sizeof(struct psw));

	/*
	 * Idle task doesn't get added back to the queue
	 */
	if (prev == &idle_task)
		goto go;

	store_pasce(&prev->regs.cr1);

	/*
	 * Add back on the queue
	 */
	prev->state = newstate;

	/*
	 * If the previous task didn't use it's full slice, force idle_task
	 * to take over.
	 */
	if (prev->slice_end_time >= (ticks + SCHED_TICKS_PER_SLICE))
		force_idle = prev->slice_end_time;

	switch(newstate) {
		case TASK_ZOMBIE:
			list_del(&prev->proc_list);
			free_pages(prev->stack, 0);
			free(prev);
			break;
		case TASK_LOCKED:
			break;
		default:
			list_add_tail(&prev->run_queue, &runnable);
			break;
	}

go:
	/*
	 * Run the rest of the scheduler that selects the next task and
	 * context switches
	 */
	__sched(force_idle);
}

/**
 * __schedule_svc - wrapper for the supervisor-service call handler
 */
void __schedule_svc(void)
{
	__schedule(SVC_INT_OLD_PSW, TASK_SLEEPING);
}

/**
 * __schedule_blocked__svc - wrapper for the supervisor-service call handler
 */
void __schedule_blocked_svc(void)
{
	__schedule(SVC_INT_OLD_PSW, TASK_LOCKED);
}

/**
 * __schedule_blocked__svc - wrapper for the supervisor-service call handler
 */
void __schedule_exit_svc(void)
{
	__schedule(SVC_INT_OLD_PSW, TASK_ZOMBIE);
}

/*
 * Initialize the schduler, and all associated structures
 */
void init_sched(void)
{
	u64 cr0;

	INIT_LIST_HEAD(&runnable);
	INIT_LIST_HEAD(&processes);

	init_idle_task();

	/* enable cpu timer interrupt subclass */
	asm volatile(
		"stctg	0,0,%0\n"	/* get cr0 */
		"oi	%1,0x04\n"	/* enable cpu timer subclass */
		"ni	%1,0x7f\n"	/* disable clock comp */
		"lctlg	0,0,%0\n"	/* reload cr0 */
	: /* output */
	: /* input */
	  "m" (cr0),
	  "m" (*(((u8*)&cr0) + 6))
	);

	set_timer();
}

void list_tasks(void (*f)(struct task*, void*), void *priv)
{
	struct task *t;

	list_for_each_entry(t, &processes, proc_list)
		f(t, priv);
}

void make_runnable(struct task *task)
{
	task->state = TASK_SLEEPING;
	list_add_tail(&task->run_queue, &runnable);
}
