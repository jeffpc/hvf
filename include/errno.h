#ifndef __ERRNO_H
#define __ERRNO_H

#define ENOMEM		1
#define EBUSY		2
#define EAGAIN		3
#define EINVAL		4
#define ESUBINVAL	5
#define EEXIST		6
#define ENOENT		7
#define EUCHECK		8

#define PTR_ERR(ptr)	((s64) ptr)
#define ERR_PTR(err)	((void*) err)

static inline int IS_ERR(void *ptr)
{
	return -1024 < PTR_ERR(ptr) && PTR_ERR(ptr) < 0;
}

#endif
