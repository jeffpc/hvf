static int cmd_begin(struct virt_sys *sys, char *cmd, int len)
{
	sys->task->cpu->state = GUEST_OPERATING;
	return 0;
}

static int cmd_stop(struct virt_sys *sys, char *cmd, int len)
{
	sys->task->cpu->state = GUEST_STOPPED;
	return 0;
}
