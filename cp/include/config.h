/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <list.h>
#include <edf.h>
#include <parser.h>

/*
 * Base address within a guest's address space; used as the base address for
 * the IPL helper code.
 *
 * NOTE: It must be >= 16M, but <2G
 */
#define GUEST_IPL_BASE		(16ULL * 1024ULL * 1024ULL)

#define CONFIG_LRECL			80
#define CONFIG_FILE_NAME		"SYSTEM  "
#define CONFIG_FILE_TYPE		"CONFIG  "

#define NSS_FILE_TYPE			"NSS     "

struct sysconf {
	u16			oper_con;
	char			oper_userid[9];
	struct list_head	rdevs;
	struct list_head	logos;
};

enum {
	LOGO_CONN_LOCAL	= 1,
};

struct logo_rec {
	struct list_head	list;
	u8			data[0];
};

struct logo {
	struct list_head	list;
	int			conn;
	u16			devtype;
	char			fn[8];
	char			ft[8];

	struct list_head	lines;
};

extern struct fs *sysfs;

/* the actual config */
extern struct sysconf sysconf;

extern struct fs *load_config();
extern int config_lex(void *data, void *yyval);
extern int config_parse(struct parser *parser);

#endif
