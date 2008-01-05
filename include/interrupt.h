#ifndef __INTERRUPT_H
#define __INTERRUPT_H

/*
 * I/O interruptions specific constants & structures
 */
struct io_int_code {
	u32 ssid;
	u32 param;
} __attribute__((packed));

#define IO_INT_OLD_PSW	((void*) 0x170)
#define IO_INT_NEW_PSW	((void*) 0x1f0)
#define IO_INT_CODE	((struct io_int_code*) 0xb8)

#endif
