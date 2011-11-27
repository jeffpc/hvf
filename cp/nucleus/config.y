%{
/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <slab.h>

static void copy_fn(char *dst, char *src)
{
	int len;

	len = strnlen(src, 8);

	memcpy(dst, src, 8);
	if (len < 8)
		memset(dst + len, ' ', 8 - len);
}

static void __logo(int local, u64 devtype, char *fn, char *ft)
{
	struct logo *logo;

	if (devtype > 0xffff)
		goto out;

	logo = malloc(sizeof(struct logo), ZONE_NORMAL);
	if (!logo)
		goto out;

	INIT_LIST_HEAD(&logo->list);
	INIT_LIST_HEAD(&logo->lines);

	/* save the <conn, devtype, fn, ft> pair */

	if (local)
		logo->conn = LOGO_CONN_LOCAL;
	else
		goto out_free;

	logo->devtype = devtype;

	copy_fn(logo->fn, fn);
	copy_fn(logo->ft, ft);

	list_add_tail(&logo->list, &sysconf.logos);

	return;

out_free:
	free(logo);
out:
	BUG();
}

static void __rdev(u64 devnum, u64 devtype)
{
	assert(devtype <= 0xffff);
	assert(devnum <= 0xffff);

	/* FIXME: save the <devnum,devtype> pair */
}

static void __oper_con(u64 devnum)
{
	assert(devnum <= 0xffff);

	sysconf.oper_con = devnum;
}

static void __oper_userid(char *name)
{
	strncpy(sysconf.oper_userid, name, 8);
	sysconf.oper_userid[8] = '\0';
}

static void __direct(char *fn, char *ft)
{
	copy_fn(sysconf.direct_fn, fn);
	copy_fn(sysconf.direct_ft, ft);
}

%}

%union {
	char *ptr;
	u64 num;
};

%token <ptr> WORD
%token <num> NUM
%token OPERATOR RDEV LOGO CONSOLE USERID LOCAL DIRECTORY
%token NLINE COMMENT

%%

stmts : stmts stmt
      | stmt
      ;

stmt : OPERATOR CONSOLE NUM NLINE	{ __oper_con($3); }
     | OPERATOR USERID WORD NLINE	{ __oper_userid($3);
					  free($3);
					}
     | OPERATOR USERID OPERATOR NLINE	{ __oper_userid("OPERATOR"); }
     | RDEV NUM NUM NLINE		{ __rdev($2, $3); }
     | LOGO LOCAL NUM WORD WORD NLINE	{ __logo(1, $3, $4, $5);
					  free($4);
					  free($5);
					}
     | LOGO LOCAL NUM WORD LOGO NLINE	{ __logo(1, $3, $4, "LOGO");
					  free($4);
					}
     | DIRECTORY WORD WORD NLINE	{ __direct($2, $3);
					  free($2);
					  free($3);
					}
     | COMMENT NLINE
     | NLINE
     ;
