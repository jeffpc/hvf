#include <device.h>
#include <bdev.h>
#include <edf.h>
#include <ebcdic.h>

static int parse_config_stmnt(char *stmnt)
{
	if (stmnt[0] == '*')
		return 0; /* comment */

	return 0;
}

int load_config(u32 iplsch)
{
	struct device *dev;
	struct fs *fs;
	struct file *file;
	char buf[CONFIG_LRECL];
	int ret;
	int i;

	/* find the real device */
	dev = find_device_by_sch(iplsch);
	if (IS_ERR(dev))
		return PTR_ERR(dev);

	/* mount the fs */
	fs = edf_mount(dev);
	if (IS_ERR(fs))
		return PTR_ERR(fs);

	/* look up the config file */
	file = edf_lookup(fs, CONFIG_FILE_NAME, CONFIG_FILE_TYPE);
	if (IS_ERR(file))
		return PTR_ERR(file);

	/* parse each record in the config file */
	for(i=0; i<file->FST.FSTAIC; i++) {
		ret = edf_read_rec(file, buf, i);
		if (ret)
			return ret;

		ebcdic2ascii((u8 *) buf, CONFIG_LRECL);

		ret = parse_config_stmnt(buf);
		if (ret)
			return ret;
	}

	return 0;
}
