/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __MUTEX_H
#define __MUTEX_H

#include <list.h>
#include <atomic.h>
#include <spinlock.h>
#include <sched.h>
#include <interrupt.h>
#include <ldep.h>

typedef struct {
	atomic_t state;
	spinlock_t queue_lock;
	struct list_head queue;
	struct lock_class *lclass;
} mutex_t;

#define UNLOCKED_MUTEX(name, lc)	\
		mutex_t name = { \
			.state = ATOMIC_INIT(1), \
			.queue = LIST_HEAD_INIT(name.queue), \
			.queue_lock = SPIN_LOCK_UNLOCKED, \
			.lclass = (lc), \
		}

static inline void mutex_init(mutex_t *lock, struct lock_class *lc)
{
	atomic_set(&lock->state, 1);
	INIT_LIST_HEAD(&lock->queue);
	lock->queue_lock = SPIN_LOCK_UNLOCKED;
	lock->lclass = lc;
}

extern void __mutex_lock_slow(mutex_t *lock);

static inline void __mutex_lock(mutex_t *lock, char *lname)
{
	/*
	 * if we are not interruptable, we shouldn't call any functions that
	 * may sleep - e.g., mutex_lock
	 */
	BUG_ON(!interruptable());

	ldep_lock(lock, lock->lclass, lname);

	if (unlikely(atomic_add_unless(&lock->state, -1, 0) == 0))
		__mutex_lock_slow(lock); /* the slow-path */
}

#define mutex_lock(l)	__mutex_lock((l), #l)

static inline void __mutex_unlock(mutex_t *lock, char *lname)
{
	struct task *task;

	ldep_unlock(lock, lname);

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

#define mutex_unlock(l)	__mutex_unlock((l), #l)

#endif
