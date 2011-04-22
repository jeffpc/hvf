/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __LDEP_H
#define __LDEP_H

#define LDEP_STACK_SIZE 10

struct held_lock {
	void *ra;		/* return address */
	void *lock;		/* the lock */
	char *lockname;		/* name of this lock */
};

struct lock_class {
	char *name;
	void *ra;
	int ndeps;
	struct lock_class **deps;
};

#define LOCK_CLASS(cname)	\
		struct lock_class cname = { \
			.name = #cname, \
			.ra = NULL, \
			.ndeps = 0, \
			.deps = NULL, \
		}

extern void ldep_on();

extern void ldep_lock(void *lock, struct lock_class *c, char *lockname);
extern void ldep_unlock(void *lock, char *lockname);
extern void ldep_no_locks();

#endif
