#include <sched.h>
#include <vcpu.h>

void queue_prog_exception(struct virt_sys *sys, enum PROG_EXCEPTION type, u64 param)
{
	con_printf(sys->con, "FIXME: supposed to inject a %d program exception\n", type);
	sys->task->cpu->state = GUEST_STOPPED;
}

