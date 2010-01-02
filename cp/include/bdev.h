/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __BDEV_H
#define __BDEV_H

extern int bdev_read_block(struct device *dev, void *buf, int lba);

#endif
