/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <sched.h>
#include <interrupt.h>

u64 svc_table[NR_SVC] = {
	(u64) __schedule_svc,
	(u64) __schedule_blocked_svc,
	(u64) __schedule_exit_svc,
};

