/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <errno.h>
#include <directory.h>

#include "directory_structs.c"

struct user *find_user_by_id(char *userid)
{
	struct user *u;

	if (!userid)
		return ERR_PTR(-ENOENT);

	u = directory;

	for (; u->userid; u++)
		if (!strcasecmp(u->userid, userid))
			return u;

	return ERR_PTR(-ENOENT);
}
