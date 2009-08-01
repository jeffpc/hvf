#ifndef __MUTEX_H
#define __MUTEX_H

#include <list.h>
#include <atomic.h>
#include <spinlock.h>
#include <sched.h>
#include <interrupt.h>

typedef struct {
	atomic_t state;
	struct list_head queue;
	spinlock_t queue_lock;
} mutex_t;

#define UNLOCKED_MUTEX(name)	mutex_t name = { \
			.state = ATOMIC_INIT(1), \
			.queue = LIST_HEAD_INIT(name.queue), \
			.queue_lock = SPIN_LOCK_UNLOCKED, \
		}

extern void __mutex_lock(mutex_t *lock);

static inline void mutex_lock(mutex_t *lock)
{
	/*
	 * if we are not interruptable, we shouldn't call any functions that
	 * may sleep - e.g., mutex_lock
	 */
	BUG_ON(!interruptable());

	if (unlikely(atomic_add_unless(&lock->state, -1, 0) == 0))
		__mutex_lock(lock); /* the slow-path */
}

static inline void mutex_unlock(mutex_t *lock)
{
	struct task *task;

	spin_lock(&lock->queue_lock);

	if (likely(list_empty(&lock->queue))) {
		/* no one is waiting on the queue */
		atomic_inc(&lock->state);
		spin_unlock(&lock->queue_lock);
		return;
	}

	/*
	 * someone is waiting on the queue, let's dequeue them & make them
	 * runnable again
	 */
	task = list_first_entry(&lock->queue, struct task, blocked_list);
	list_del(&task->blocked_list);
	spin_unlock(&lock->queue_lock);

	make_runnable(task);
}

#endif
