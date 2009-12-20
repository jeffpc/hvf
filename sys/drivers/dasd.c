#include <device.h>
#include <console.h>
#include <list.h>
#include <io.h>
#include <sched.h>

static int d3390_snprintf(struct device *dev, char* buf, int len)
{
	return snprintf(buf, len, "%10d CYL ", dev->eckd.cyls);
}

static inline unsigned int ceil_quot(unsigned int d1, unsigned int d2)
{
        return (d1 + (d2 - 1)) / d2;
}

static inline int d3390_recs_per_track(int kl, int dl)
{
	int dn, kn;

	dn = ceil_quot(dl + 6, 232) + 1;
	if (kl) {
		kn = ceil_quot(kl + 6, 232) + 1;
		return 1729 / (10 + 9 + ceil_quot(kl + 6 * kn, 34) +
			       9 + ceil_quot(dl + 6 * dn, 34));
	} else
		return 1729 / (10 + 9 + ceil_quot(dl + 6 * dn, 34));
}

static int d3390_reg(struct device *dev)
{
	struct io_op ioop;
	struct ccw ccw;
	int ret;
	u8 buf[64];

	switch(dev->model) {
		case 0x02: /* 3390, 3390-1 */
		case 0x06: /* 3390-2 */
		case 0x0a: /* 3390-3 */
		case 0x0c: /* 3390-9, 3390-27, 3390-J, 3390-54, 3390-JJ */
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
	ioop.orb.addr = ADDR31(&ccw);
	ioop.orb.f = 1;

	memset(&ccw, 0, sizeof(struct ccw));
	ccw.cmd = 0x64; /* RDC */
	ccw.flags = CCW_FLAG_SLI;
	ccw.count = 64;
	ccw.addr = ADDR31(buf);

	/*
	 * issue RDC
	 */
	ret = submit_io(dev, &ioop, CAN_LOOP);
	if (ret)
		return ret;

	dev->eckd.cyls    = (buf[12] << 8) |
		buf[13];
	dev->eckd.tracks  = (buf[14] << 8) |
		buf[15];
	dev->eckd.recs    = d3390_recs_per_track(0, 4096);
	dev->eckd.sectors = buf[16];
	dev->eckd.len     = (buf[18] << 8) |
		buf[19];

	dev->eckd.formula = buf[22];
	if (dev->eckd.formula == 1) {
		dev->eckd.f1 = buf[23];
		dev->eckd.f2 = (buf[24] << 8) |
			buf[25];
		dev->eckd.f3 = (buf[26] << 8) |
			buf[27];
		dev->eckd.f4 = 0;
		dev->eckd.f5 = 0;
	} else if (dev->eckd.formula == 2) {
		dev->eckd.f1 = buf[23];
		dev->eckd.f2 = buf[24];
		dev->eckd.f3 = buf[25];
		dev->eckd.f4 = buf[26];
		dev->eckd.f5 = buf[27];
	} else
		return -EINVAL;

	return 0;
}

static int d3390_read(struct device *dev, u8 *buf, int lba)
{
	struct io_op ioop;
	struct ccw ccw[4];
	u8 seek_data[6];
	u8 search_data[5];
	int ret;

	u16 cc, hh, r;
	int rpt = dev->eckd.recs;
	int rpc = dev->eckd.tracks * rpt;

	cc = lba / rpc;
	hh = (lba % rpc) / rpt;
	r = 1 + ((lba % rpc) % rpt);

	/*
	 * Set up IO op
	 */
	ioop.handler = NULL;
	ioop.dtor = NULL;

	memset(&ioop.orb, 0, sizeof(struct orb));
	ioop.orb.lpm = 0xff;
	ioop.orb.addr = ADDR31(ccw);
	ioop.orb.f = 1;

	memset(ccw, 0, sizeof(ccw));

	/* SEEK */
	ccw[0].cmd = 0x07;
	ccw[0].flags = CCW_FLAG_CC | CCW_FLAG_SLI;
	ccw[0].count = 6;
	ccw[0].addr = ADDR31(seek_data);

	seek_data[0] = 0;		/* zero */
	seek_data[1] = 0;		/* zero */
	seek_data[2] = cc >> 8;		/* Cc */
	seek_data[3] = cc & 0xff;	/* cC */
	seek_data[4] = hh >> 8;		/* Hh */
	seek_data[5] = hh & 0xff;	/* hH */

	/* SEARCH */
	ccw[1].cmd = 0x31;
	ccw[1].flags = CCW_FLAG_CC | CCW_FLAG_SLI;
	ccw[1].count = 5;
	ccw[1].addr = ADDR31(search_data);

	search_data[0] = cc >> 8;
	search_data[1] = cc & 0xff;
	search_data[2] = hh >> 8;
	search_data[3] = hh & 0xff;
	search_data[4] = r;

	/* TIC */
	ccw[2].cmd = 0x08;
	ccw[2].flags = 0;
	ccw[2].count = 0;
	ccw[2].addr = ADDR31(&ccw[1]);

	/* READ DATA */
	ccw[3].cmd = 0x86;
	ccw[3].flags = 0;
	ccw[3].count = 4096;
	ccw[3].addr = ADDR31(buf);

	/*
	 * issue IO
	 */
	ret = submit_io(dev, &ioop, CAN_SLEEP);
	return ret;
}

static struct device_type d3390 = {
	.types		= LIST_HEAD_INIT(d3390.types),
	.reg		= d3390_reg,
	.interrupt	= NULL,
	.enable		= NULL,
	.snprintf	= d3390_snprintf,

	.read		= d3390_read,

	.type		= 0x3390,
	.all_models	= 1,
};

/******************************************************************************/

static int d9336_snprintf(struct device *dev, char* buf, int len)
{
	return snprintf(buf, len, "%10d BLK ", dev->fba.blks);
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
	ioop.orb.addr = ADDR31(&ccw);
	ioop.orb.f = 1;

	memset(&ccw, 0, sizeof(struct ccw));
	ccw.cmd = 0x64; /* RDC */
	ccw.flags = CCW_FLAG_SLI;
	ccw.count = 64;
	ccw.addr = ADDR31(buf);

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
	.enable		= NULL,
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

