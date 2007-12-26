#include <sched.h>
#include <page.h>
#include <buddy.h>

/* list of all runnable processes */
static struct list_head runnable;

/* list of all processes in the system */
static struct list_head processes;

/* user & nucleus tasks */
static struct task tasks[MAX_PROCESSES];

/* idle task */
static struct task idle_task;

/*
 * Clear a struct task
 */
static void __reset_task(struct task *task)
{
	memset(task, 0, sizeof(struct task));
}

static void idle_task_body()
{
	struct psw psw;
	
	memset(&psw, 0, sizeof(struct psw));

	psw.io = 1;
	psw.m  = 1;
	psw.w  = 1;
	psw.ea = 1;
	psw.ba = 1;

	/* FIXME: use a magic number that doesn't change */
	psw.ptr = (u64) &idle_task_body;

	/* load a wait psw */
	lpswe(&psw);

	/* control should never reach here */

	BUG();
}

/*
 * Initialize the idle task
 */
static void init_idle_task()
{
	struct page *page;

	__reset_task(&idle_task);
	
	/* allocate stack */
	page = alloc_pages(0);
	BUG_ON(!page);

	/* stack */
	idle_task.regs.gpr[15] = (u64) (PAGE_SIZE + (u8*)page_to_addr(page));

	/* 
	 * FIXME:
	 *
	 * keep track of the allocated page, and make sure that we don't
	 * theoretically leak it
	 *
	 * We should add it into an address space structure of some sort,
	 * and then have page table entries work their magic
	 */
}

/*
 * Initialize the schduler, and all associated structures
 */
void init_sched()
{
	int i;

	INIT_LIST_HEAD(&runnable);
	INIT_LIST_HEAD(&processes);

	for(i=0; i<MAX_PROCESSES; i++)
		__reset_task(&tasks[i]);

	init_idle_task();
}

