/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <device.h>
#include <console.h>
#include <list.h>
#include <sched.h>
#include <directory.h>

static struct device_type d3215 = {
	.types		= LIST_HEAD_INIT(d3215.types),
	.reg		= NULL,
	.interrupt	= NULL,
	.enable		= NULL,
	.snprintf	= NULL,
	.type		= 0x3215,
	.model		= 0,
};

int register_driver_3215(void)
{
	return register_device_type(&d3215);
}

