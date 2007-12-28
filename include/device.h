#ifndef __DEVICE_H
#define __DEVICE_H

#include <list.h>

struct device_type {
	struct list_head types;
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

extern int register_device_type(struct device_type *dev);
extern void scan_devices();
extern void register_drivers();

#endif
