#include <device.h>
#include <bdev.h>

int bdev_read_block(struct device *dev, void *buf, int lba)
{
	if (!dev->dev->read)
		return -EINVAL;

	return dev->dev->read(dev, buf, lba);
}
