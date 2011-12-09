/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <vcpu.h>

int guest_begin(struct virt_sys *sys)
{
	switch(sys->cpu->state) {
		case GUEST_STOPPED:
		case GUEST_OPERATING:
			sys->cpu->state = GUEST_OPERATING;
			return 0;
		default:
			return -EINVAL;
	}
}

int guest_stop(struct virt_sys *sys)
{
	switch(sys->cpu->state) {
		case GUEST_STOPPED:
		case GUEST_OPERATING:
			sys->cpu->state = GUEST_STOPPED;
			return 0;
		default:
			return -EINVAL;
	}
}
