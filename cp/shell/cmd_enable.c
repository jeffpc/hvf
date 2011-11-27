/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

/*
 *!!! ENABLE
 *!! SYNTAX
 *! \tok{\sc ENAble}
 *! \begin{stack} \tok{ALL} \\ <rdev> \end{stack}
 *!! XATNYS
 *!! AUTH A
 *!! PURPOSE
 *! Enables a real device.
 */
static int cmd_enable(struct virt_sys *sys, char *cmd, int len)
{
	u64 devnum;
	struct device *dev;
	struct console *con;

	SHELL_CMD_AUTH(sys, A);

	if (!strcasecmp(cmd, "ALL")) {
		con_printf(sys->con, "ENABLE ALL not yet implemented!\n");
		return 0;
	}

	cmd = __extract_hex(cmd, &devnum);
	if (IS_ERR(cmd))
		return PTR_ERR(cmd);

	/* device number must be 16-bit */
	if (devnum & ~0xffffUL)
		return -EINVAL;

	dev = find_device_by_ccuu(devnum);
	if (IS_ERR(dev)) {
		con_printf(sys->con, "Device %04llX not found in configuration\n", devnum);
		return 0;
	}

	if (atomic_read(&dev->in_use)) {
		con_printf(sys->con, "Device %04llX is already in use\n", devnum);
		dev_put(dev);
		return 0;
	}

	if (!dev->dev->enable) {
		con_printf(sys->con, "Device type %-4s cannot be enabled\n",
			   type2name(dev->type));
		return 0;
	}

	con = dev->dev->enable(dev);
	if (IS_ERR(con)) {
		con_printf(sys->con, "Failed to enable %04llX\n", devnum);
		return PTR_ERR(con);
	}

	return 0;
}
