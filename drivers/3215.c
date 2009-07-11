#include <device.h>
#include <console.h>
#include <list.h>

static struct device_type d3215 = {
	.types		= LIST_HEAD_INIT(d3215.types),
	.reg		= register_console,
	.interrupt	= NULL,
	.snprintf	= NULL,
	.type		= 0x3215,
	.model		= 0,
};

int register_driver_3215(void)
{
	return register_device_type(&d3215);
}

