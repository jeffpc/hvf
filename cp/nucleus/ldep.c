/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <sclp.h>
#include <sched.h>
#include <list.h>
#include <spinlock.h>
#include <ldep.h>

static spinlock_t __lock = SPIN_LOCK_UNLOCKED;
static int ldep_enabled;

void ldep_on()
{
	unsigned long mask;

	spin_lock_intsave(&__lock, &mask);
	ldep_enabled = 1;
	spin_unlock_intrestore(&__lock, mask);
}

static int __get_stack_slot()
{
	current->nr_locks++;

	if (current->nr_locks == LDEP_STACK_SIZE) {
		sclp_msg("task '%s' exceeded the number of tracked "
			 "locks (%d)! disabling ldep!\n", current->name,
			 LDEP_STACK_SIZE);
		return 1;
	}

	return 0;
}

static void ldep_warn_head(char *lockname, void *addr)
{
	sclp_msg("task '%s' is trying to acquire lock:\n", current->name);
	sclp_msg(" (%s), at: %p\n\n", lockname, addr);
}

static void ldep_warn_recursive(char *lockname, void *addr, struct held_lock *held)
{
	sclp_msg("[INFO: possible recursive locking detected]\n");

	ldep_warn_head(lockname, addr);

	sclp_msg("but task is already holding lock:\n\n");
	sclp_msg(" (%s), at: %p\n\n", held->lockname, held->ra);
}

static void print_held_locks()
{
	struct held_lock *cur;
	int i;

	sclp_msg("\nlocks currently held:\n");
	for(i=0; i<current->nr_locks; i++) {
		cur = &current->lock_stack[i];
		sclp_msg(" #%d:   (%s), at %p\n", i, cur->lockname, cur->ra);
	}
}

void ldep_lock(void *lock, struct lock_class *c, char *lockname)
{
	void *ra = __builtin_return_address(0);
	struct held_lock *cur;
	unsigned long mask;
//	int ret;
	int i;

	// LOCK
	spin_lock_intsave(&__lock, &mask);

	if (!ldep_enabled)
		goto out;

	if (current->nr_locks) {
		/* check for recursive locking */
		for(i=0; i<current->nr_locks; i++) {
			cur = &current->lock_stack[i];
			if (cur->lclass != c)
				continue;

			ldep_warn_recursive(lockname, ra, cur);
			ldep_enabled = 0;
			goto out;
		}
	}

	/* ok, no issues, add the lock we're trying to get to the stack */
	if (__get_stack_slot())
		goto out;

	cur = &current->lock_stack[current->nr_locks-1];
	cur->ra = ra;
	cur->lock = lock;
	cur->lockname = lockname;
	cur->lclass = c;

out:
	// UNLOCK
	spin_unlock_intrestore(&__lock, mask);
}

void ldep_unlock(void *lock, char *lockname)
{
	void *ra = __builtin_return_address(0);
	struct held_lock *cur;
	unsigned long mask;
	int i;

	// LOCK
	spin_lock_intsave(&__lock, &mask);

	if (!ldep_enabled)
		goto out;

	for(i=0; i<current->nr_locks; i++) {
		cur = &current->lock_stack[i];
		if (cur->lock == lock)
			goto found;
	}

	sclp_msg("task '%s' is trying to release lock it doesn't have:\n",
		 current->name);
	sclp_msg(" (%s), at %p\n", lockname, ra);
	print_held_locks();

	ldep_enabled = 0;

	goto out;

found:
	if (i != current->nr_locks-1)
		memcpy(&current->lock_stack[i],
		       &current->lock_stack[i+1],
		       current->nr_locks - i - 1);

	current->nr_locks--;

out:
	// UNLOCK
	spin_unlock_intrestore(&__lock, mask);
}

void ldep_no_locks()
{
	void *ra = __builtin_return_address(0);
	unsigned long mask;

	// LOCK
	spin_lock_intsave(&__lock, &mask);

	if (!ldep_enabled)
		goto out;

	if (!current->nr_locks)
		goto out;

	sclp_msg("task '%s' is holding a lock when it shouldn't have:\n",
		  current->name);
	sclp_msg(" at %p\n", ra);
	print_held_locks();

	ldep_enabled = 0;

out:
	// UNLOCK
	spin_unlock_intrestore(&__lock, mask);
}
