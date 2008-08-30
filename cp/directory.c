#include <errno.h>
#include <directory.h>

static struct user directory[] = {
	{
		.userid = "operator",
		.con = NULL,
		.storage_size = 2 * 1024 * 1024,
		.devices = {
			{
				/* CON */
				.type = VDEV_CONS,
				.vdev = 0x0009,
				.vsch = 0x10000,
			},
			{
				/* RDR */
				.type = VDEV_SPOOL,
				.vdev = 0x000C,
				.vsch = 0x10001,
				.u.spool = { .type = 0x3505, .model = 1, },
			},
			{
				/* PUN */
				.type = VDEV_SPOOL,
				.vdev = 0x000D,
				.vsch = 0x10002,
				.u.spool = { .type = 0x3525, .model = 1, },
			},
			{
				/* PRT */
				.type = VDEV_SPOOL,
				.vdev = 0x000E,
				.vsch = 0x10003,
				.u.spool = { .type = 0x1403, .model = 1, },
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
		.con = NULL,
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
