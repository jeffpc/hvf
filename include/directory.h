#ifndef __DIRECTORY_H
#define __DIRECTORY_H

#include <console.h>

enum vdevtype {
	VDEV_INVAL = 0,			/* invalid */
	VDEV_CONS,			/* a console */
	VDEV_DED,			/* dedicated real device */
	VDEV_SPOOL,			/* backed by the spool */
	VDEV_MDISK,			/* a minidisk */
	VDEV_LINK,			/* a link to another user's device */
};

struct vdev {
	enum vdevtype type;		/* device type */
	u16 vdev;			/* virtual dev # */
	u32 vsch;			/* virtual sch # */

	union {
		/* VDEV_CONS */
		struct {
		} console;

		/* VDEV_DED */
		struct {
		} dedicate;

		/* VDEV_SPOOL */
		struct {
			u16 type;
			u8 model;
		} spool;

		/* VDEV_MDISK */
		struct {
		} mdisk;

		/* VDEV_LINK */
		struct {
		} link;
	} u;
};

struct user {
	char *userid;
	struct console *con;

	/* VM configuration */
	u64 storage_size;

	/* FIXME: make this dynamically allocated */
	struct vdev devices[5];
};

extern struct user *find_user_by_id(char *userid);

#endif
