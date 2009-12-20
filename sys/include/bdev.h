#ifndef __BDEV_H
#define __BDEV_H

extern int bdev_read_block(struct device *dev, u8 *buf, int lba);

#endif
