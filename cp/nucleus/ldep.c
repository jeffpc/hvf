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

#define PBSIZE PAGE_SIZE
static char pbuf[PBSIZE];
static int plen;

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
		plen += snprintf(pbuf+plen, PBSIZE-plen,
				 "task '%s' exceeded the number of tracked "
				 "locks (%d)! disabling ldep!\n", current->name,
				 LDEP_STACK_SIZE);
		return 1;
	}

	return 0;
}

static void ldep_warn_head(char *lockname, void *addr)
{
	plen += snprintf(pbuf+plen, PBSIZE-plen,
			 "task '%s' is trying to acquire lock:\n",
			 current->name);
	plen += snprintf(pbuf+plen, PBSIZE-plen,
			 " (%s), at: %p\n\n", lockname, addr);
}

static void ldep_warn_recursive(char *lockname, void *addr, struct held_lock *held)
{
	plen = 0;

	plen += snprintf(pbuf+plen, PBSIZE-plen,
			 "[INFO: possible recursive locking detected]\n");

	ldep_warn_head(lockname, addr);

	plen += snprintf(pbuf+plen, PBSIZE-plen,
			 "but task is already holding lock:\n\n");
	plen += snprintf(pbuf+plen, PBSIZE-plen,
			 " (%s), at: %p\n\n", held->lockname, held->ra);
}

static void print_held_locks()
{
	struct held_lock *cur;
	int i;

	plen += snprintf(pbuf+plen, PBSIZE-plen,
			 "\nlocks currently held:\n");
	for(i=0; i<current->nr_locks; i++) {
		cur = &current->lock_stack[i];
		plen += snprintf(pbuf+plen, PBSIZE-plen,
				 " #%d:   (%s), at %p\n", i, cur->lockname,
				 cur->ra);
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
			BUG();
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

	plen = 0;
	plen += snprintf(pbuf+plen, PBSIZE-plen,
			 "task '%s' is trying to release lock it doesn't have:\n",
			 current->name);
	plen += snprintf(pbuf+plen, PBSIZE-plen,
			 " (%s), at %p\n", lockname, ra);
	print_held_locks();
	BUG();

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

	plen = 0;
	plen += snprintf(pbuf+plen, PBSIZE-plen,
			 "task '%s' is holding a lock when it shouldn't have:\n",
			 current->name);
	plen += snprintf(pbuf+plen, PBSIZE-plen,
			 " at %p\n", ra);
	print_held_locks();
	BUG();

	ldep_enabled = 0;

out:
	// UNLOCK
	spin_unlock_intrestore(&__lock, mask);
}
