/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * Base address within a guest's address space; used as the base address for
 * the IPL helper code.
 *
 * NOTE: It must be >= 16M, but <2G
 */
#define GUEST_IPL_BASE		(16ULL * 1024ULL * 1024ULL)

#define OPER_CONSOLE_CCUU	0x0009

#define CONFIG_LRECL			80
#define CONFIG_FILE_NAME		"SYSTEM  "
#define CONFIG_FILE_TYPE		"CONFIG  "

extern int load_config();

#endif
