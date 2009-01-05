static int cmd_begin(struct user *u, char *cmd, int len)
{
	current->guest->state = GUEST_OPERATING;
	return 0;
}

static int cmd_stop(struct user *u, char *cmd, int len)
{
	current->guest->state = GUEST_STOPPED;
	return 0;
}
