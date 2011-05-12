/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __DIRECTORY_H
#define __DIRECTORY_H

#include <console.h>

enum directory_vdevtype {
	VDEV_INVAL = 0,			/* invalid */
	VDEV_CONS,			/* a console */
	VDEV_DED,			/* dedicated real device */
	VDEV_SPOOL,			/* backed by the spool */
	VDEV_MDISK,			/* a minidisk */
	VDEV_LINK,			/* a link to another user's device */
};

struct directory_vdev {
	enum directory_vdevtype type;	/* device type */
	u16 vdev;			/* virtual dev # */

	union {
		/* VDEV_CONS */
		struct {
		} console;

		/* VDEV_DED */
		struct {
			u16 rdev;	/* real device # */
		} dedicate;

		/* VDEV_SPOOL */
		struct {
			u16 type;
			u8 model;
		} spool;

		/* VDEV_MDISK */
		struct {
			u16 cyloff;	/* first cylinder # */
			u16 cylcnt;	/* cylinder count */
			u16 rdev;	/* real DASD containing the mdisk */
		} mdisk;

		/* VDEV_LINK */
		struct {
		} link;
	} u;
};

struct user {
	char *userid;
	struct task *task;

	/* VM configuration */
	u64 storage_size;

	struct directory_vdev *devices;

	u8 auth;
};

extern struct user *find_user_by_id(char *userid);

#endif
