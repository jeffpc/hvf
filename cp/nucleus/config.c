/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <device.h>
#include <bdev.h>
#include <edf.h>
#include <slab.h>
#include <directory.h>
#include <ebcdic.h>
#include <sclp.h>
#include <parser.h>
#include <lexer.h>

struct sysconf sysconf;

static void __load_logos(struct fs *fs)
{
	struct logo *logo, *tmp;
	struct logo_rec *rec;
	struct file *file;
	int ret;
	int i;

	list_for_each_entry_safe(logo, tmp, &sysconf.logos, list) {
		/* look up the config file */
		file = edf_lookup(fs, logo->fn, logo->ft);
		if (IS_ERR(file))
			goto remove;

		if ((file->FST.LRECL != CONFIG_LRECL) ||
		    (file->FST.RECFM != FSTDFIX))
			goto remove;

		/* parse each record in the config file */
		for(i=0; i<file->FST.AIC; i++) {
			rec = malloc(sizeof(struct logo_rec) + CONFIG_LRECL,
				     ZONE_NORMAL);
			if (!rec)
				goto remove;

			ret = edf_read_rec(file, (char*) rec->data, i);
			if (ret)
				goto remove;

			ebcdic2ascii(rec->data, CONFIG_LRECL);

			list_add_tail(&rec->list, &logo->lines);
		}

		continue;

remove:
		list_del(&logo->list);
		free(logo);
	}
}

static void *_realloc(void *ptr, size_t n)
{
	if (!ptr)
		return malloc(n, ZONE_NORMAL);

	BUG();
	return NULL;
}

static void _error(struct parser *parser, char *msg)
{
	sclp_msg("config parse error: %s\n", msg);
	BUG();
}

struct fs *load_config(u32 iplsch)
{
	struct parser p;
	struct lexer l;
	struct device *dev;
	struct fs *fs;

	sclp_msg("LOADING CONFIG FROM    '%*.*s' '%*.*s'\n",
		 8, 8, CONFIG_FILE_NAME, 8, 8, CONFIG_FILE_TYPE);

	memset(&sysconf, 0, sizeof(struct sysconf));
	INIT_LIST_HEAD(&sysconf.rdevs);
	INIT_LIST_HEAD(&sysconf.logos);

	/* find the real device */
	dev = find_device_by_sch(iplsch);
	if (IS_ERR(dev))
		return ERR_CAST(dev);

	/* mount the fs */
	fs = edf_mount(dev);
	if (IS_ERR(fs))
		return fs;

	/* set up the parser */
	memset(&p, 0, sizeof(p));
	p.lex = config_lex;
	p.lex_data = &l;
	p.realloc = _realloc;
	p.free = free;
	p.error = _error;

	/* set up the lexer */
	memset(&l, 0, sizeof(l));
	l.fs = fs;
	l.init = 0;

	/* parse! */
	if (config_parse(&p))
		return ERR_PTR(-ECORRUPT);

	load_directory(fs);
	__load_logos(fs);

	return fs;
}
