#include <list.h>
#include <channel.h>
#include <io.h>
#include <slab.h>
#include <device.h>
#include <spinlock.h>
#include <sched.h>

struct senseid_struct {
	u8 __reserved;
	u16 cu_type;
	u8 cu_model;
	u16 dev_type;
	u8 dev_model;
} __attribute__((packed));

struct static_device {
	u16 dev_num;
	struct senseid_struct sense;
};

#define END_OF_STATIC_DEV_LIST	0xffff

/*
 * This defines staticly-configured devices - ugly but necessary for devices
 * that fail to identify themseleves via Sense-ID
 */
static struct static_device static_device_list[] = {
	{ .dev_num = 0x0009, .sense = { .dev_type = 0x3215, .dev_model = 0 } },
	{ .dev_num = END_OF_STATIC_DEV_LIST },
};

/*
 * We need this device temporarily because submit_io & friends assume that
 * the device that's doing IO is on the device list. Unfortunately, that
 * isn't the case during IO device scanning. We register this device type,
 * and each device temporarily during the scan to make submit_io & friends
 * happy. When we're done scanning, we unregister this type.
 */
static struct device_type __fake_dev_type = {
	.types		= LIST_HEAD_INIT(__fake_dev_type.types),
	.reg		= NULL,
	.interrupt	= NULL,
	.type		= 0,
	.model		= 0,
};

static LIST_HEAD(device_types);
static spinlock_t dev_types_lock;

static LIST_HEAD(devices);
static spinlock_t devs_lock;

static void unregister_device_type(struct device_type *type)
{
	spin_lock(&dev_types_lock);
	list_del(&type->types);
	spin_unlock(&dev_types_lock);
}

/**
 * register_device_type - register a new device type/model
 * @dev:	device type to register
 */
int register_device_type(struct device_type *dev)
{
	struct device_type *entry;

	spin_lock(&dev_types_lock);

	list_for_each_entry(entry, &device_types, types) {
		if (dev == entry ||
		    (dev->type == entry->type && dev->model == entry->model)) {
			spin_unlock(&dev_types_lock);
			return -EEXIST;
		}
	}

	list_add_tail(&dev->types, &device_types);

	spin_unlock(&dev_types_lock);

	return 0;
}

/**
 * find_device_by_ccuu - find device struct by ccuu
 * @ccuu:	device ccuu to find
 */
struct device *find_device_by_ccuu(u16 ccuu)
{
	struct device *dev;

	spin_lock(&devs_lock);

	list_for_each_entry(dev, &devices, devices) {
		if (dev->ccuu == ccuu) {
			spin_unlock(&devs_lock);
			return dev;
		}
	}

	spin_unlock(&devs_lock);

	return ERR_PTR(-ENOENT);
}

/**
 * find_device_by_sch - find device struct by subchannel number
 * @sch:	device subchannel
 */
struct device *find_device_by_sch(u32 sch)
{
	struct device *dev;

	spin_lock(&devs_lock);

	list_for_each_entry(dev, &devices, devices) {
		if (dev->sch == sch) {
			spin_unlock(&devs_lock);
			return dev;
		}
	}

	spin_unlock(&devs_lock);

	return ERR_PTR(-ENOENT);
}

/**
 * find_device_by_type - find device struct by type/model
 * @type:	device type to find
 * @model:	device model to find
 */
struct device *find_device_by_type(u16 type, u8 model)
{
	struct device *dev;

	spin_lock(&devs_lock);

	list_for_each_entry(dev, &devices, devices) {
		if (dev->type == type &&
		    dev->model == model) {
			spin_unlock(&devs_lock);
			return dev;
		}
	}

	spin_unlock(&devs_lock);

	return ERR_PTR(-ENOENT);
}

static void __unregister_device(struct device *dev)
{
	unsigned long mask;

	spin_lock_intsave(&dev->q_lock, &mask);
	BUG_ON(!list_empty(&dev->q_out) ||
	       atomic_read(&dev->attention) ||
	       dev->q_cur);
	spin_unlock_intrestore(&dev->q_lock, mask);

	spin_lock(&devs_lock);
	list_del(&dev->devices);
	spin_unlock(&devs_lock);
}

/**
 * __register_device - helper to register a device
 * @dev:	device to register
 */
static int __register_device(struct device *dev)
{
	struct device_type *type;
	int err = 0;

	spin_double_lock(&devs_lock, &dev_types_lock);

	list_for_each_entry(type, &device_types, types) {
		if (type->type == dev->type &&
		    type->model == dev->model)
			goto found;
	}

	err = -ENOENT;
	type = NULL;

found:
	dev->dev = type;

	if (type && type->reg)
		err = type->reg(dev);

	list_add_tail(&dev->devices, &devices);

	spin_double_unlock(&devs_lock, &dev_types_lock);

	return err;
}

static int do_sense_id(struct device *dev, u16 dev_num, struct senseid_struct *buf)
{
	struct io_op ioop;
	struct ccw ccw;
	int ret;
	int idx;
	struct static_device *sdev;

	/*
	 * Check static configuration; if device is found (by device
	 * number), use that information instead of issuing sense-id
	 */
	for(idx = 0; sdev = &static_device_list[idx],
	    sdev->dev_num != END_OF_STATIC_DEV_LIST; idx++) {
		if (sdev->dev_num == dev_num) {
			memcpy(buf, &sdev->sense, sizeof(struct senseid_struct));
			return 0;
		}
	}

	/*
	 * Set up IO op for Sense-ID
	 */
	ioop.handler = NULL;
	ioop.dtor = NULL;

	memset(&ioop.orb, 0, sizeof(struct orb));
	ioop.orb.lpm = 0xff;
	ioop.orb.addr = (u32) (u64) &ccw;
	ioop.orb.f = 1;

	memset(&ccw, 0, sizeof(struct ccw));
	ccw.cmd = 0xe4; /* Sense-ID */
	ccw.sli = 1;
	ccw.count = sizeof(struct senseid_struct);
	ccw.addr = (u32) (u64) buf;

	/*
	 * issue SENSE-ID
	 */
	ret = submit_io(dev, &ioop, CAN_LOOP);
	BUG_ON(ret);

	return ioop.err;
}

/*
 * Scan all subchannel ids, and register each device
 */
void scan_devices(void)
{
	struct schib schib;
	struct device *dev = NULL;
	struct senseid_struct buf;
	u32 sch;
	int ret;

	BUG_ON(register_device_type(&__fake_dev_type));

	memset(&schib, 0, sizeof(struct schib));

	/*
	 * For each possible subchannel id...
	 */
	for(sch = 0x10000; sch <= 0x1ffff; sch++) {
		/*
		 * ...call store subchannel, to find out whether or not
		 * there is a device
		 */
		if (store_sch(sch, &schib))
			continue;

		if (!schib.path_ctl.v)
			continue;

		/*
		 * The following code tries to take the following steps:
		 *   - alloc device struct
		 *   - enable the subchannel
		 *   - MSCH
		 *   - issue SENSE-ID IO op & wait for completion
		 *   - register device with apropriate subsystem
		 */

		if (!dev) {
			dev = malloc(sizeof(struct device), ZONE_NORMAL);

			/*
			 * if we failed to allocate memory, there's not much we can
			 * do
			 */
			BUG_ON(!dev);

			atomic_set(&dev->attention, 0);
			INIT_LIST_HEAD(&dev->q_out);
		}

		schib.path_ctl.e = 1;

		if (modify_sch(sch, &schib))
			continue;

		dev->sch   = sch;
		BUG_ON(__register_device(dev));
		BUG_ON(dev->dev != &__fake_dev_type);

		/*
		 * Find out what the device is - whichever way is necessary
		 */
		ret = do_sense_id(dev, schib.path_ctl.dev_num, &buf);

		__unregister_device(dev); /* it's fake! */

		if (ret)
			continue;

		dev->type  = buf.dev_type;
		dev->model = buf.dev_model;
		dev->ccuu  = schib.path_ctl.dev_num;

		if (__register_device(dev)) {
			/*
			 * error registering ... the device struct MUST NOT
			 * be freed as it has been added onto the devices
			 * list. All that needs to be done is to reset the
			 * enabled bit.
			 */
			schib.path_ctl.e = 0;

			/* msch could fail, but it shouldn't be fatal */
			modify_sch(sch, &schib);
		}

		/* to prevent a valid device struct from being free'd */
		dev = NULL;
	}

	unregister_device_type(&__fake_dev_type);

	free(dev);
}

void list_devices(struct console *con, void (*f)(struct console*, struct device*))
{
	struct device *dev;

	list_for_each_entry(dev, &devices, devices)
		f(con, dev);
}

void register_drivers(void)
{
	register_driver_3215();
}
