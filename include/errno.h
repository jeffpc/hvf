/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __ERRNO_H
#define __ERRNO_H

#define SUCCESS		0
#define ENOMEM		1
#define EBUSY		2
#define EAGAIN		3
#define EINVAL		4
#define EEXIST		5
#define ENOENT		6
#define ESUBENOENT	7
#define EUCHECK		8
#define EFAULT		9
#define EPERM		10
#define ECORRUPT	11

#define PTR_ERR(ptr)	((s64) ptr)
#define ERR_PTR(err)	((void*) (long) err)
#define ERR_CAST(err)	((void*) err)

static inline int IS_ERR(void *ptr)
{
	return -1024 < PTR_ERR(ptr) && PTR_ERR(ptr) < 0;
}

extern char *errstrings[];

#endif
