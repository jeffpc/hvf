#ifndef __GUEST_H
#define __GUEST_H

#include <vcpu.h>
#include <vdevice.h>

struct virt_sys *guest_create(char *name, struct device *rcon);
int guest_ipl_nss(struct virt_sys *sys, char *nssname);
int guest_begin(struct virt_sys *sys);
int guest_stop(struct virt_sys *sys);

extern void list_users(struct virt_cons *con, void (*f)(struct virt_cons *con,
							struct virt_sys *sys));

extern int spool_exec(struct virt_sys *sys, struct virt_device *vdev);

#endif
