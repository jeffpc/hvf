#include <sched.h>
#include <interrupt.h>

u64 svc_table[NR_SVC] = {
	(u64) __schedule_svc,
	(u64) __schedule_blocked_svc,
};

