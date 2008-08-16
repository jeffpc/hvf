#include <errno.h>
#include <directory.h>

static struct user directory[] = {
	{
		.userid = "operator",
		.con = NULL,
		.storage_size = 2 * 1024 * 1024,
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
