#include <errno.h>

char *errstrings[] = {
	[ENOMEM]     = "Out of memory",
	[EBUSY]      = "Resource busy",
	[EAGAIN]     = "Try again",
	[EINVAL]     = "Invalid argument",
	[EEXIST]     = "File already exists",
	[ENOENT]     = "No such file",
	[ESUBENOENT] = "?",
	[EUCHECK]    = "?",
	[EFAULT]     = "Invalid memory reference",
	[EPERM]      = "Permission denied",
	[ECORRUPT]   = "Curruption detected",
};
