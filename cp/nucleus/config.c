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
#include <ebcdic.h>

struct sysconf sysconf;

int __get_u16(char *s, u16 *d)
{
	u16 ret = 0;

	if (!*s)
		return -1;

	for(; *s; s++) {
		if ((*s >= '0') && (*s <= '9'))
			ret = (ret << 4) | (*s - '0');
		else if ((*s >= 'A') && (*s <= 'F'))
			ret = (ret << 4) | (*s - 'A' + 10);
		else {
			BUG();
			return -2;
		}
	}

	*d = ret;

	return 0;
}

static int __parse_operator(char *s)
{
	char *t1, *t2;

	if ((t1 = strsep(&s, " ")) == NULL)
		return -EINVAL;

	if ((t2 = strsep(&s, " ")) == NULL)
		return -EINVAL;

	if (!strcmp(t1, "CONSOLE")) {
		return __get_u16(t2, &sysconf.oper_con);
	} else if (!strcmp(t1, "USERID")) {
		strncpy(sysconf.oper_userid, t2, 8);
		sysconf.oper_userid[8] = '\0';
	} else
		return -EINVAL;

	return 0;
}

static int __parse_rdev(char *s)
{
	u16 devnum, devtype;
	int ret;

	char *t1, *t2;

	if ((t1 = strsep(&s, " ")) == NULL)
		return -EINVAL;

	if ((t2 = strsep(&s, " ")) == NULL)
		return -EINVAL;

	ret = __get_u16(t1, &devnum);
	if (ret)
		return ret;

	ret = __get_u16(t2, &devtype);
	if (ret)
		return ret;

	/* FIXME: save the <devnum,devtype> pair */

	return 0;
}

static void copy_fn(char *dst, char *src)
{
	int len;

	len = strnlen(src, 8);

	memcpy(dst, src, 8);
	if (len < 8)
		memset(dst + len, ' ', 8 - len);
}

static int __parse_logo(char *s)
{
	char *conn, *_devtype, *fn, *ft;
	struct logo *logo;
	u16 devtype;
	int ret;

	if ((conn = strsep(&s, " ")) == NULL)
		return -EINVAL;

	if ((_devtype = strsep(&s, " ")) == NULL)
		return -EINVAL;

	ret = __get_u16(_devtype, &devtype);
	if (ret)
		return ret;

	if ((fn = strsep(&s, " ")) == NULL)
		return -EINVAL;

	if ((ft = strsep(&s, " ")) == NULL)
		return -EINVAL;

	logo = malloc(sizeof(struct logo), ZONE_NORMAL);
	if (!logo)
		return -ENOMEM;

	INIT_LIST_HEAD(&logo->list);
	INIT_LIST_HEAD(&logo->lines);

	/* save the <conn, devtype, fn, ft> pair */

	if (!strcmp(conn, "LOCAL"))
		logo->conn = LOGO_CONN_LOCAL;
	else
		goto out_free;

	logo->devtype = devtype;

	copy_fn(logo->fn, fn);
	copy_fn(logo->ft, ft);

	list_add_tail(&logo->list, &sysconf.logos);

	return 0;

out_free:
	free(logo);
	return -ECORRUPT;
}

int parse_config_stmnt(char *stmnt)
{
	char *s = stmnt;
	char *tok;

	if (stmnt[0] == '*')
		return 0; /* comment */

	if (stmnt[0] == '\0')
		return 0; /* empty line */

	if ((tok = strsep(&s, " ")) != NULL) {
		if (!strcmp(tok, "OPERATOR"))
			return __parse_operator(s);
		else if (!strcmp(tok, "RDEV"))
			return __parse_rdev(s);
		else if (!strcmp(tok, "LOGO"))
			return __parse_logo(s);
		else
			BUG();
	}

	BUG();
	return -EINVAL;
}

static void null_terminate(char *s, int lrecl)
{
	int i;

	for(i=lrecl-1; i>=0; i--)
		if (s[i] != ' ')
			break;

	s[i+1] = '\0';
}

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

struct fs *load_config(u32 iplsch)
{
	char buf[CONFIG_LRECL+1];
	struct device *dev;
	struct fs *fs;
	struct file *file;
	int ret;
	int i;

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

	/* look up the config file */
	file = edf_lookup(fs, CONFIG_FILE_NAME, CONFIG_FILE_TYPE);
	if (IS_ERR(file))
		return ERR_CAST(file);

	if (file->FST.LRECL != CONFIG_LRECL)
		return ERR_PTR(-EINVAL);

	/* parse each record in the config file */
	for(i=0; i<file->FST.AIC; i++) {
		ret = edf_read_rec(file, buf, i);
		if (ret)
			return ERR_PTR(ret);

		ebcdic2ascii((u8 *) buf, CONFIG_LRECL);

		ascii2upper((u8 *) buf, CONFIG_LRECL);

		null_terminate(buf, CONFIG_LRECL);

		ret = parse_config_stmnt(buf);
		if (ret)
			return ERR_PTR(ret);
	}

	__load_logos(fs);

	return fs;
}
