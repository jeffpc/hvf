#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * Base address within a guest's address space; used as the base address for
 * the IPL helper code.
 *
 * NOTE: It must be >= 16M, but <2G
 */
#define GUEST_IPL_BASE		(16ULL * 1024ULL * 1024ULL)

#define GUEST_IPL_DEVNUM	0x0a00
#define GUEST_IPL_SCHNUM	0x10005

#endif
