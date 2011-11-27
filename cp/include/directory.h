/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __DIRECTORY_H
#define __DIRECTORY_H

#include <console.h>

#define AUTH_A		0x80 /* operations (shutdown, force, ...) */
#define AUTH_B		0x40 /* device op (attach, detach) */
#define AUTH_C		0x20 /* sys prog (alter real storage) */
#define AUTH_D		0x10 /* spool op (start/drain UR, access all spools) */
#define AUTH_E		0x08 /* system analyst (examine real storage) */
#define AUTH_F		0x04 /* CE (I/O device error analysis) */
#define AUTH_G		0x02 /* general user */

/* only useful during directory load */
struct directory_prop {
	int got_storage;
	u64 storage;
};

enum directory_vdevtype {
	VDEV_INVAL = 0,			/* invalid */
	VDEV_CONS,			/* a console */
	VDEV_DED,			/* dedicated real device */
	VDEV_SPOOL,			/* backed by the spool */
	VDEV_MDISK,			/* a minidisk */
	VDEV_LINK,			/* a link to another user's device */
};

struct directory_vdev {
	struct list_head list;

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
	struct list_head list;		/* list of users */

	char *userid;
	struct task *task;

	/* VM configuration */
	u64 storage_size;

	struct list_head devices;

	u8 auth;
};

extern struct user *find_user_by_id(char *userid);
extern void directory_alloc_user(char *name, int auth,
				 struct directory_prop *prop,
				 struct list_head *vdevs);
extern int load_directory(struct fs *fs);
extern int direct_lex(void *data, void *yyval);
extern int direct_parse(struct parser *parser);

#endif
