/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <device.h>
#include <bdev.h>

int bdev_read_block(struct device *dev, void *buf, int lba)
{
	if (!dev->dev->read)
		return -EINVAL;

	return dev->dev->read(dev, buf, lba);
}
