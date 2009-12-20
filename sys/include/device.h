#ifndef __DEVICE_H
#define __DEVICE_H

#include <atomic.h>
#include <spinlock.h>
#include <list.h>

struct device;
struct irb;

struct device_type {
	struct list_head types;
	int (*reg)(struct device *dev);
	int (*interrupt)(struct device *dev, struct irb *irb);
	void* (*enable)(struct device *dev);
	int (*snprintf)(struct device *dev, char *buf, int len);

	/* the following ops are for the bdev wrapper layer */
	int (*read)(struct device *dev, u8 *buf, int len);

	u16 type;
	u8 model;
	u8 all_models;
};

struct device {
	struct list_head devices;
	u32 sch;			/* subchannel id */
	u16 type;			/* 3330, 3215, ... */
	u8 model;
	u16 ccuu;			/* device number */
	struct device_type *dev;

	spinlock_t q_lock;
	struct io_op *q_cur;
	struct list_head q_out;
	atomic_t attention;

	atomic_t in_use;		/* is the device in use by someone? */

	atomic_t refcnt;		/* internal reference counter */

	union {
		struct {
			u16 cyls;	/* cylinders */
			u16 tracks;	/* tracks per cylinder */
			u16 len;	/* track length */
			u8  sectors;	/* # sectors per track */
			u8  formula;	/* capacity formula */
			u16 f1,		/* factor f1 */
			    f2,		/* factor f2 */
			    f3,		/* factor f3 */
			    f4,		/* factor f4 */
			    f5;		/* factor f5 */
		} eckd;
		struct {
			u16 blk_size;	/* block size */
			u32 bpg;	/* blocks per cyclical group */
			u32 bpp;	/* blocks per access possition */
			u32 blks;	/* blocks */
		} fba;
	};
};

extern struct device *find_device_by_type(u16 type, u8 model);
extern struct device *find_device_by_ccuu(u16 ccuu);
extern struct device *find_device_by_sch(u32 sch);
extern int register_device_type(struct device_type *dev);
extern void scan_devices(void);
extern void list_devices(struct console *con,
			 void (*f)(struct console*, struct device*));
extern void register_drivers(void);

static inline void dev_get(struct device *dev)
{
	BUG_ON(atomic_read(&dev->refcnt) < 1);
	atomic_inc(&dev->refcnt);
}

static inline void dev_put(struct device *dev)
{
	atomic_dec(&dev->refcnt);
	BUG_ON(atomic_read(&dev->refcnt) < 1);
}

/* device specific register functions */
extern int register_driver_3215(void);
extern int register_driver_dasd(void);

#endif
