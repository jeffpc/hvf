#include <device.h>
#include <console.h>
#include <list.h>
#include <io.h>
#include <sched.h>

static struct device_type d3390 = {
	.types		= LIST_HEAD_INIT(d3390.types),
	.reg		= NULL,
	.interrupt	= NULL,
	.enable		= NULL,
	.snprintf	= NULL,
	.type		= 0x3390,
	.all_models	= 1,
};

/******************************************************************************/

static struct device_type d9336 = {
	.types		= LIST_HEAD_INIT(d9336.types),
	.reg		= NULL,
	.interrupt	= NULL,
	.enable		= NULL,
	.snprintf	= NULL,
	.type		= 0x9336,
	.all_models	= 1,
};

int register_driver_dasd(void)
{
	int ret;

	ret = register_device_type(&d3390);
	if (ret)
		return ret;

	return register_device_type(&d9336);
}

