#ifndef __BDEV_H
#define __BDEV_H

extern int bdev_read_block(struct device *dev, void *buf, int lba);

#endif
