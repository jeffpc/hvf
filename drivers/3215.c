#include <device.h>
#include <console.h>
#include <list.h>
#include <sched.h>
#include <directory.h>

static int d3215_snprintf(struct device *dev, char* buf, int len)
{
	struct console *con;

	if (!buf || !len)
		return 0;

	con = find_console(dev);
	if (IS_ERR(con))
		return 0;

	if (con->sys)
		return snprintf(buf, len, "LOGON AS %s ",
				con->sys->directory->userid);

	return 0;
}

static struct device_type d3215 = {
	.types		= LIST_HEAD_INIT(d3215.types),
	.reg		= register_console,
	.interrupt	= NULL,
	.snprintf	= d3215_snprintf,
	.type		= 0x3215,
	.model		= 0,
};

int register_driver_3215(void)
{
	return register_device_type(&d3215);
}

