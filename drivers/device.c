#include <list.h>
#include <channel.h>
#include <io.h>
#include <slab.h>
#include <device.h>
#include <spinlock.h>

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


static LIST_HEAD(device_types);
static spinlock_t dev_types_lock;

static LIST_HEAD(devices);
static spinlock_t devs_lock;

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

static int do_sense_id(u32 sch, u16 dev_num, struct senseid_struct *buf)
{
	struct io_op ioop;
	struct ccw ccw;
	int ret;
	int idx;
	struct static_device *dev;

	/*
	 * Check static configuration; if device is found (by device
	 * number), use that information instead of issuing sense-id
	 */
	for(idx = 0; dev = &static_device_list[idx],
	    dev->dev_num != END_OF_STATIC_DEV_LIST; idx++) {
		if (dev->dev_num == dev_num) {
			memcpy(buf, &dev->sense, sizeof(struct senseid_struct));
			return 0;
		}
	}

	/*
	 * Set up IO op for Sense-ID
	 */
	ioop.ssid = sch;
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
	ret = submit_io(&ioop, 0);
	BUG_ON(ret);

	/* FIXME: this is a very nasty hack */
	while(!atomic_read(&ioop.done))
		;

	return ioop.err;
}

/*
 * Scan all subchannel ids, and register each device
 */
void scan_devices()
{
	struct schib schib;
	struct device *dev = NULL;
	struct senseid_struct buf;
	u32 sch;

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
		 *   - set interrupt_param (FIXME)
		 *   - MSCH
		 *   - issue SENSE-ID IO op
		 *   - wait for completion (busy-wait) (FIXME)
		 *   - register device with apropriate subsystem
		 */

		if (!dev)
			dev = malloc(sizeof(struct device), ZONE_NORMAL);
		/*
		 * if we failed to allocate memory, there's not much we can
		 * do
		 */
		BUG_ON(!dev);

		schib.path_ctl.e = 1;

		if (modify_sch(sch, &schib))
			continue;

		/*
		 * Find out what the device is - whichever way is necessary
		 */
		if (do_sense_id(sch, schib.path_ctl.dev_num, &buf))
			continue;

		dev->sch   = sch;
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

	free(dev);
}

void list_devices(struct console *con)
{
	struct device *dev;

	list_for_each_entry(dev, &devices, devices) {
		con_printf(con, "    %04x-%02d @ %04x (sch %05x)\n", dev->type,
		       dev->model, dev->ccuu, dev->sch);
	}
}

extern int register_driver_3215();

void register_drivers()
{
	register_driver_3215();
}
