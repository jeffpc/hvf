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
 * idle_task_body - body of the idle task (a wait psw)
 */
static int idle_task_body()
{
	struct psw psw;
	
	memset(&psw, 0, sizeof(struct psw));

	psw.io = 1;
	psw.w  = 1;
	psw.ea = 1;
	psw.ba = 1;

	psw.ptr = MAGIC_PSW_IDLE_CODE;

	/* load a wait psw */
	lpswe(&psw);

	/* control should never reach here */

	BUG();

	return 1;
}

/**
 * start_task - helper used to start the task's code
 * @f:	function to execute
 */
static void start_task(int (*f)())
{
	/*
	 * Start executing the code
	 */
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
	task->regs.psw.ea	= 1;
	task->regs.psw.ba	= 1;

	/* code to execute */
	task->regs.psw.ptr = (u64) start_task;

	task->regs.gpr[2]  = (u64) f;
	task->regs.gpr[15] = ((u64) stack) + PAGE_SIZE - STACK_FRAME_SIZE;

	task->state = TASK_SLEEPING;
}

/**
 * init_idle_task - initialize the idle task
 */
static void init_idle_task()
{
	struct page *page;

	page = alloc_pages(0);
	BUG_ON(!page);

	__init_task(&idle_task, idle_task_body, page_to_addr(page));

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

