#include <device.h>
#include <console.h>
#include <list.h>
#include <io.h>
#include <sched.h>

static int d3390_snprintf(struct device *dev, char* buf, int len)
{
	int cyls = 0;

	switch(dev->model) {
		case 0x0c: cyls =  3339; break; /* 3390-3 */
		case 0x0a: cyls = 10017; break; /* 3390-9 */
	}

	return snprintf(buf, len, "%10d CYL ", cyls);
}

static int d3390_reg(struct device *dev)
{
	switch(dev->model) {
		case 0x0c: return 0; /* 3390-3 */
		case 0x0a: return 0; /* 3390-9 */
	}

	return -ENOENT;
}

static struct device_type d3390 = {
	.types		= LIST_HEAD_INIT(d3390.types),
	.reg		= d3390_reg,
	.interrupt	= NULL,
	.snprintf	= d3390_snprintf,
	.type		= 0x3390,
	.all_models	= 1,
};

/******************************************************************************/

static int d9336_snprintf(struct device *dev, char* buf, int len)
{
	return snprintf(buf, len, "%13d BLK ", dev->fba.blks);
}

static int d9336_reg(struct device *dev)
{
	struct io_op ioop;
	struct ccw ccw;
	int ret;
	u8 buf[64];

	switch(dev->model) {
		case 0x00: /* 9336-10 */
		case 0x10: /* 9336-20 */
			break;
		default:
			return -ENOENT;
	}

	/*
	 * Set up IO op for Read Device Characteristics
	 */
	ioop.handler = NULL;
	ioop.dtor = NULL;

	memset(&ioop.orb, 0, sizeof(struct orb));
	ioop.orb.lpm = 0xff;
	ioop.orb.addr = (u32) (u64) &ccw;
	ioop.orb.f = 1;

	memset(&ccw, 0, sizeof(struct ccw));
	ccw.cmd = 0x64; /* RDC */
	ccw.sli = 1;
	ccw.count = 64;
	ccw.addr = (u32) (u64) buf;

	/*
	 * issue RDC
	 */
	ret = submit_io(dev, &ioop, CAN_LOOP);
	if (ret)
		return ret;

	dev->fba.blk_size = (buf[4] << 8) | buf[5];
	dev->fba.bpg = (buf[6] << 24)  |
	               (buf[7] << 16)  |
		       (buf[8] << 8)   |
		       buf[9];
	dev->fba.bpp = (buf[10] << 24) |
	               (buf[11] << 16) |
		       (buf[12] << 8)  |
	               buf[13];
	dev->fba.blks= (buf[14] << 24) |
	               (buf[15] << 16) |
		       (buf[16] << 8)  |
	               buf[17];

	return 0;
}

static struct device_type d9336 = {
	.types		= LIST_HEAD_INIT(d9336.types),
	.reg		= d9336_reg,
	.interrupt	= NULL,
	.snprintf	= d9336_snprintf,
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

