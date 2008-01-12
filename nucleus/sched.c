#include <sched.h>
#include <page.h>
#include <buddy.h>
#include <slab.h>
#include <magic.h>
#include <interrupt.h>

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
static void start_task(int (*f)())
{
	/*
	 * Start executing the code
	 */
	if (f)
		(*f)();

	/*
	 * Done, now, it's time to cleanup
	 *
	 * FIXME:
	 *  - delete from processes list
	 *  - free stack page
	 *  - free struct
	 *  - schedule
	 */
	BUG();
}

/**
 * __init_task - helper to initialize the task structure
 * @task:	task struct to initialize
 * @f:		pointer to function to execute
 * @stack:	pointer to the page for stack
 */
static void __init_task(struct task *task, void *f, void *stack)
{
	memset(task, 0, sizeof(struct task));

	task->regs.psw.io	= 1;
	task->regs.psw.ex	= 1;
	task->regs.psw.ea	= 1;
	task->regs.psw.ba	= 1;

	/* code to execute */
	task->regs.psw.ptr = (u64) start_task;

	task->regs.gpr[2]  = (u64) f;
	task->regs.gpr[15] = ((u64) stack) + PAGE_SIZE - sizeof(void*) -
			STACK_FRAME_SIZE;

	/*
	 * Last 8 bytes of the stack page contain a pointer to the task
	 * structure
	 */
	set_task_ptr(stack, task);

	task->state = TASK_SLEEPING;
}

/**
 * init_idle_task - initialize the idle task
 *
 * Since the initial thread of execution already has a stack, and the task
 * struct is a global variable, all that is left for us to do is to
 * associate the current stack with idle_task.
 */
static void init_idle_task()
{
	u64 stack;

	stack = ((u64) &stack) & ~(PAGE_SIZE-1);

	__init_task(&idle_task, NULL, (void*) stack);

	/*
	 * NOTE: The idle task is _not_ supposed to be on either of the
	 * two process lists.
	 */
}

/**
 * create_task - create a new task
 * @f:	function pointer to where thread of execution should begin
 */
int create_task(int (*f)())
{
	struct page *page;
	struct task *task;
	int err = -ENOMEM;

	/*
	 * Allocate a page for stack, and struct task itself
	 */
	page = alloc_pages(0);
	task = malloc(sizeof(struct task));
	if (!page || !task)
		goto out;

	/*
	 * Set up task's state
	 */
	__init_task(task, f, page_to_addr(page));

	/*
	 * Add the task to the scheduler lists
	 */
	list_add_tail(&task->proc_list, &processes);
	list_add_tail(&task->run_queue, &runnable);

	return 0;

out:
	free(task);
	free_pages(page_to_addr(page), 0);

	return err;
}

/**
 * __sched - core of the scheduler code. This decides which task to run
 * next, and switches over to it
 */
static void __sched()
{
	struct task *next;

	/*
	 * If there are no runnable tasks, let's use the idle thread
	 */
	if (list_empty(&runnable)) {
		next = &idle_task;
		goto run;
	}

	next = list_first_entry(&runnable, struct task, run_queue);

	/*
	 * Remove the new task from the run queue & mark it as running
	 */
	list_del(&next->run_queue);
	next->state = TASK_RUNNING;

run:
	/*
	 * NOTE: Because we need to load the registers _BEFORE_ we issue
	 * lpswe, we have to place the new psw into PSA and use register 0
	 */
	memcpy(PSA_TMP_PSW, &next->regs.psw, sizeof(struct psw));

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
void __schedule(struct psw *old_psw)
{
	struct task *prev;

	/*
	 * Save last process's state
	 */

	prev = extract_task_ptr((void*) PSA_INT_GPR[15]);
	
	/*
	 * Save the registers (gpr, psw, ...)
	 *
	 * FIXME: fp? ar? cr?
	 */
	memcpy(prev->regs.gpr, PSA_INT_GPR, sizeof(u64)*16);
	memcpy(&prev->regs.psw, old_psw, sizeof(struct psw));

	/*
	 * Add back on the queue
	 *
	 * FIXME: should we try to be fair, and have partial slices?
	 */
	if (prev != &idle_task)
		list_add_tail(&prev->run_queue, &runnable);

	/*
	 * Run the rest of the scheduler that selects the next task and
	 * context switches
	 */
	__sched();
}

/**
 * __schedule_svc - wrapper for the supervisor-service call handler
 */
void __schedule_svc()
{
	__schedule(SVC_INT_OLD_PSW);
}

/**
 * schedule - used to explicitly yield the cpu
 */
void schedule()
{
	asm volatile(
		"	svc	%0\n"
	: /* output */
	: /* input */
	  "i" (SVC_SCHEDULE)
	);
}

/*
 * Initialize the schduler, and all associated structures
 */
void init_sched()
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

