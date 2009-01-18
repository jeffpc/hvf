#ifndef __DEVICE_H
#define __DEVICE_H

#include <list.h>

struct device;
struct irb;

struct device_type {
	struct list_head types;
	int (*reg)(struct device *dev);
	int (*interrupt)(struct device *dev, struct irb *irb);
	u16 type;
	u8 model;
};

struct device {
	struct list_head devices;
	u32 sch;			/* subchannel id */
	u16 type;			/* 3330, 3215, ... */
	u8 model;
	u16 ccuu;			/* device number */
	struct device_type *dev;
};

extern struct device *find_device_by_type(u16 type, u8 model);
extern struct device *find_device_by_ccuu(u16 ccuu);
extern struct device *find_device_by_sch(u32 sch);
extern int register_device_type(struct device_type *dev);
extern void scan_devices(void);
extern void list_devices(struct console *con,
			 void (*f)(struct console*, struct device*));
extern void register_drivers(void);

/* device specific register functions */
extern int register_driver_3215(void);

#endif
