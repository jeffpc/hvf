/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <sched.h>
#include <vcpu.h>

void queue_prog_exception(struct virt_sys *sys, enum PROG_EXCEPTION type, u64 param)
{
	con_printf(sys->con, "FIXME: supposed to inject a %d program exception\n", type);
	sys->task->cpu->state = GUEST_STOPPED;
}

