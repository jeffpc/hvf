/*
 *!!! BEGIN
 *!! SYNTAX
 *! \tok{\sc BEgin}
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Starts/resumes virtual machine execution.
 */
static int cmd_begin(struct virt_sys *sys, char *cmd, int len)
{
	sys->task->cpu->state = GUEST_OPERATING;
	return 0;
}

/*
 *!!! STOP
 *!! SYNTAX
 *! \tok{\sc STOP}
 *!! XATNYS
 *!! AUTH G
 *!! PURPOSE
 *! Stops virtual machine execution.
 */
static int cmd_stop(struct virt_sys *sys, char *cmd, int len)
{
	sys->task->cpu->state = GUEST_STOPPED;
	return 0;
}
