/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <sched.h>
#include <vcpu.h>
#include <slab.h>

void queue_prog_exception(struct virt_sys *sys, enum PROG_EXCEPTION type, u64 param)
{
	FIXME("supposed to inject a %d program exception", type);
	con_printf(sys->con, "FIXME: supposed to inject a %d program exception\n", type);
	sys->cpu->state = GUEST_STOPPED;
}

void queue_io_interrupt(struct virt_sys *sys, u32 ssid, u32 param, u32 a, u32 isc)
{
	struct vio_int *ioint;

	BUG_ON(isc >= 8);
	BUG_ON((a != 0) && (a != 1));

	ioint = malloc(sizeof(struct vio_int), ZONE_NORMAL);
	BUG_ON(!ioint);

	ioint->ssid  = ssid;
	ioint->param = param;
	ioint->intid = (a << 31) | (isc << 27);

	mutex_lock(&sys->cpu->int_lock);
	list_add_tail(&ioint->list, &sys->cpu->int_io[isc]);

	con_printf(sys->con, "queued I/O int %d %08x.%08x.%08x\n", isc,
		   ioint->ssid, ioint->param, ioint->intid);
	mutex_unlock(&sys->cpu->int_lock);
}
