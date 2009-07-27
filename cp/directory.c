#include <errno.h>
#include <directory.h>

static struct user directory[] = {
	{
		.userid = "OPERATOR",
		.storage_size = 17 * 1024 * 1024,
		.devices = {
			{
				/* CON */
				.type = VDEV_CONS,
				.vdev = 0x0009,
			},
			{
				/* RDR */
				.type = VDEV_SPOOL,
				.vdev = 0x000C,
				.u.spool = { .type = 0x3505, .model = 1, },
			},
			{
				/* PUN */
				.type = VDEV_SPOOL,
				.vdev = 0x000D,
				.u.spool = { .type = 0x3525, .model = 1, },
			},
			{
				/* PRT */
				.type = VDEV_SPOOL,
				.vdev = 0x000E,
				.u.spool = { .type = 0x1403, .model = 1, },
			},
			{
				/* 191 DASD */
				.type = VDEV_MDISK,
				.vdev = 0x0191,
				.u.mdisk = { .cyloff = 15, .cylcnt = 100, .rdev = 0x0192, },
			},
			{
				/* END */
				.type = VDEV_INVAL,
			},
		},
	},

	/* a NULL record to mark the end of the directory */
	{
		.userid = NULL,
	}
};

struct user *find_user_by_id(char *userid)
{
	struct user *u;

	if (!userid)
		return ERR_PTR(-ENOENT);

	u = directory;

	for (; u->userid; u++)
		if (!strcmp(u->userid, userid))
			return u;

	return ERR_PTR(-ENOENT);
}
