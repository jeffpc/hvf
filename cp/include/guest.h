/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __GUEST_H
#define __GUEST_H

/* All the different ways to reset the system */
extern void guest_power_on_reset(struct virt_sys *sys);
extern void guest_system_reset_normal(struct virt_sys *sys);
extern void guest_system_reset_clear(struct virt_sys *sys);
extern void guest_load_normal(struct virt_sys *sys);
extern void guest_load_clear(struct virt_sys *sys);

extern void alloc_guest_devices(struct virt_sys *sys);
extern int alloc_guest_storage(struct virt_sys *sys);
extern int alloc_vcpu(struct virt_sys *sys);
extern void free_vcpu(struct virt_sys *sys);

extern void run_guest(struct virt_sys *sys);
extern void handle_interception(struct virt_sys *sys);

extern int handle_instruction(struct virt_sys *sys);
extern int handle_instruction_priv(struct virt_sys *sys);

typedef int (*intercept_handler_t)(struct virt_sys *sys);

#endif
