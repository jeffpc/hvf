#include <list.h>
#include <channel.h>
#include <io.h>
#include <slab.h>
#include <device.h>

struct senseid_struct {
	u8 __reserved;
	u16 cu_type;
	u8 cu_model;
	u16 dev_type;
	u8 dev_model;
} __attribute__((packed));

static LIST_HEAD(device_types);
static LIST_HEAD(devices);

/**
 * register_device_type - register a new device type/model
 * @dev:	device type to register
 */
int register_device_type(struct device_type *dev)
{
	struct device_type *entry;

	list_for_each_entry(entry, &device_types, types) {
		if (dev == entry ||
		    (dev->type == entry->type && dev->model == entry->model))
			return -EEXIST;
	}

	list_add_tail(&dev->types, &device_types);

	return 0;
}

/**
 * __register_device - helper to register a device
 * @dev:	device to register
 */
static int __register_device(struct device *dev)
{
	struct device_type *type;
	int err = 0;

	list_for_each_entry(type, &device_types, types) {
		if (type->type == dev->type &&
		    type->model == dev->model)
			goto found;
	}

	err = -ENOENT;
	type = NULL;

found:
	dev->dev = type;
	list_add_tail(&dev->devices, &devices);

	return err;
}

/*
 * Scan all subchannel ids, and register each device
 */
void scan_devices()
{
	struct schib schib;
	struct device *dev = NULL;
	struct io_op ioop;
	struct ccw ccw;
	u32 sch;
	struct senseid_struct buf;
	int ret;

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
			dev = malloc(sizeof(struct device));
		/*
		 * if we failed to allocate memory, there's not much we can
		 * do
		 */
		BUG_ON(!dev);

		schib.path_ctl.e = 1;

		if (modify_sch(sch, &schib))
			continue;

		/*
		 * issue SENSE-ID
		 */
		ioop.ssid = sch;
		ioop.handler = NULL;
		ioop.dtor = NULL;

		memset(&ioop.orb, 0, sizeof(struct orb));
		ioop.orb.lpm = 0xff;
		ioop.orb.addr = (u32) (u64) &ccw;
		ioop.orb.f = 1;

		memset(&ccw, 0, sizeof(struct ccw));
		ccw.cmd = 0xe4; // sense id
		ccw.sli = 1;
		ccw.count = sizeof(struct senseid_struct);
		ccw.addr = (u32) (u64) &buf;

		ret = submit_io(&ioop, 0);
		BUG_ON(ret);

		/* FIXME: this is a very nasty hack */
		while(!atomic_read(&ioop.done))
			;

		if (ioop.err)
			continue;

		dev->sch   = sch;
		dev->type  = buf.dev_type;
		dev->model = buf.dev_model;
		dev->ccuu  = schib.path_ctl.dev_num;

		printf("    %04x-%d @ %04x (sch %05x)", dev->type,
					dev->model, dev->ccuu, dev->sch);

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

extern int register_driver_3215();

void register_drivers()
{
	register_driver_3215();
}