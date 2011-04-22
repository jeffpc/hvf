/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

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
		con_printf(NULL, "task '%s' exceeded the number of tracked "
			   "locks (%d)! disabling ldep!\n", current->name,
			   LDEP_STACK_SIZE);
		return 1;
	}

	return 0;
}

static void print_held_locks()
{
	struct held_lock *cur;
	int i;

	con_printf(NULL, "\nlocks currently held:\n");
	for(i=0; i<current->nr_locks; i++) {
		cur = &current->lock_stack[i];
		con_printf(NULL, " #%d:   (%s), at %p\n", i, cur->lockname,
		       cur->ra);
	}
}

void ldep_lock(void *lock, struct lock_class *c, char *lockname)
{
	void *ra = __builtin_return_address(0);
	struct held_lock *cur;
	unsigned long mask;

	// LOCK
	spin_lock_intsave(&__lock, &mask);

	if (!ldep_enabled)
		goto out;

	/* ok, no issues, add the lock we're trying to get to the stack */
	if (__get_stack_slot())
		goto out;

	cur = &current->lock_stack[current->nr_locks-1];
	cur->ra = ra;
	cur->lock = lock;
	cur->lockname = lockname;

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

	con_printf(NULL, "task '%s' is trying to release lock it doesn't have:\n",
		   current->name);
	con_printf(NULL, " (%s), at %p\n", lockname, ra);
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

	con_printf(NULL, "task '%s' is holding a lock when it shouldn't have:\n",
		   current->name);
	con_printf(NULL, " at %p\n", ra);
	print_held_locks();

	ldep_enabled = 0;

out:
	// UNLOCK
	spin_unlock_intrestore(&__lock, mask);
}
