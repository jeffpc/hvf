#ifndef __INTERRUPT_H
#define __INTERRUPT_H

/*
 * I/O interruptions specific constants & structures
 */
struct io_int_code {
	u32 ssid;
	u32 param;
} __attribute__((packed));

#define PSA_INT_GPR	((u64*) 0x200)
#define PSA_TMP_PSW	((struct psw*) 0x280)

#define IO_INT_OLD_PSW	((void*) 0x170)
#define IO_INT_NEW_PSW	((void*) 0x1f0)
#define IO_INT_CODE	((struct io_int_code*) 0xb8)

#define EXT_INT_OLD_PSW	((void*) 0x130)
#define EXT_INT_NEW_PSW	((void*) 0x1b0)
#define EXT_INT_CODE	((u16*) 0x86)

#define SVC_INT_OLD_PSW ((void*) 0x140)
#define SVC_INT_NEW_PSW ((void*) 0x1c0)
#define SVC_INT_CODE	((u16*) 0x8a)

#define PGM_INT_OLD_PSW	((void*) 0x150)
#define PGM_INT_NEW_PSW ((void*) 0x1d0)
#define PGM_INT_ILC	((u8*) 0x8d)
#define PGM_INT_CODE	((u16*) 0x8e)

/*
 * Assembly stubs to call the C-handlers
 */
extern void IO_INT(void);
extern void EXT_INT(void);
extern void SVC_INT(void);
extern void PGM_INT(void);

/**
 * local_int_disable - disable interruptions & return old mask
 */
#define local_int_disable() ({ \
	unsigned long __flags; \
	__asm__ __volatile__ ( \
		"stnsm 0(%1),0xfc" : "=m" (__flags) : "a" (&__flags) ); \
	__flags; \
	})

/**
 * local_int_restore - restore interrupt mask
 * x:	mask to restore
 */
#define local_int_restore(x) \
	__asm__ __volatile__("ssm   0(%0)" : : "a" (&x), "m" (x) : "memory")

extern void set_timer(void);

/*
 * The Supervisor-Service call table
 */
#define SVC_SCHEDULE		0
#define NR_SVC			1
extern u64 svc_table[NR_SVC];

/* Interrupt handlers */
extern void __pgm_int_handler(void);
extern void __ext_int_handler(void);
extern void __io_int_handler(void);

/* Interrupt handler stack pointer */
extern u8 *int_stack_ptr;

#endif
