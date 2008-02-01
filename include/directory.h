#ifndef __DIRECTORY_H
#define __DIRECTORY_H

#include <console.h>

struct user {
	char *userid;
	struct console *con;

	/* VM configuration */
	u64 storage_size;
};

extern struct user *find_user_by_id(char *userid);

#endif
