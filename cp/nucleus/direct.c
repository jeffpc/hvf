/*
 * (C) Copyright 2007-2019  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <errno.h>
#include <ebcdic.h>
#include <directory.h>
#include <shell.h>
#include <slab.h>
#include <parser.h>
#include <lexer.h>
#include <edf.h>
#include <sclp.h>

#include "direct_grammar.h"

static LIST_HEAD(directory);

int direct_parse(struct parser *);

struct user *find_user_by_id(char *userid)
{
	struct user *u;

	if (!userid)
		return ERR_PTR(-ENOENT);

	list_for_each_entry(u, &directory, list)
		if (!strcasecmp(u->userid, userid))
			return u;

	return ERR_PTR(-ENOENT);
}

static void *_realloc(void *p, size_t s)
{
	if (!p)
		return malloc(s, ZONE_NORMAL);

	if (s <= allocsize(p))
		return p;

	BUG();
	return NULL;
}

static void _error(struct parser *p, char *msg)
{
	sclp_msg("directory parse error: %s\n", msg);
	BUG();
}

int load_directory(struct fs *fs)
{
	struct parser p;
	struct lexer l;

	sclp_msg("LOADING DIRECTORY FROM '%*.*s' '%*.*s'\n",
		 8, 8, sysconf.direct_fn, 8, 8, sysconf.direct_ft);

	/* set up the parser */
	memset(&p, 0, sizeof(p));
	p.lex = direct_lex;
	p.lex_data = &l;
	p.realloc = _realloc;
	p.free = free;
	p.error = _error;

	/* set up the lexer */
	memset(&l, 0, sizeof(l));
	l.fs = fs;
	l.init = 0;

	/* parse! */
	return direct_parse(&p) ? -ECORRUPT : 0;
}

void directory_alloc_user(char *name, int auth, struct directory_prop *prop,
			  struct list_head *vdevs)
{
	struct user *user;

	assert(name);
	assert(prop->got_storage);

	user = malloc(sizeof(struct user), ZONE_NORMAL);
	assert(user);

	memset(user, 0, sizeof(struct user));

	INIT_LIST_HEAD(&user->list);
	INIT_LIST_HEAD(&user->devices);
	list_splice(vdevs, &user->devices);

	user->userid = name;
	user->storage_size = prop->storage;
	user->auth = auth;

	FIXME("locking?");
	list_add_tail(&user->list, &directory);
}
