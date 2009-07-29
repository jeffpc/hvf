#ifndef __VCPU_H
#define __VCPU_H

#include <sched.h>

/*****************************************************************************/
/* Guest exception queuing                                                   */

enum PROG_EXCEPTION {
	PROG_OPERAND		= 0x0001,
	PROG_PRIV		= 0x0002,
	PROG_EXEC		= 0x0003,
	PROG_PROT		= 0x0004,
	PROG_ADDR		= 0x0005,
	PROG_SPEC		= 0x0006,
	PROG_DATA		= 0x0007,
};

extern void queue_prog_exception(struct virt_sys *sys, enum PROG_EXCEPTION type, u64 param);

#endif
