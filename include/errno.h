#ifndef __ERRNO_H
#define __ERRNO_H

#define ENOMEM		1
#define EBUSY		2
#define EAGAIN		3

#define PTR_ERR(ptr)	((s64) ptr)
#define ERR_PTR(err)	((void*) err)

#endif
