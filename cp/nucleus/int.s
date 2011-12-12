/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

.text

#include <interrupt.h>

#
# IO Interrupt
#
	.align	4
.globl IO_INT
	.type	IO_INT, @function
IO_INT:
	# save regs
	STMG	%r0,%r15,0x200(%r0)

	# set up stack
	LARL	%r15, int_stack_ptr
	LG	%r15, 0(%r15)
	AGHI	%r15,-160

	LARL	%r14, __io_int_handler
	BALR	%r14, %r14		# call the real handler

	# restore regs
	LMG	%r0,%r15,0x200(%r0)

	LPSWE	0x170			# go back to what we interrupted


#
# External Interrupt
#
	.align 4
.globl EXT_INT
	.type	EXT_INT, @function
EXT_INT:
	# save regs
	STMG	%r0,%r15,0x200(%r0)

	# set up stack
	LARL	%r15, int_stack_ptr
	LG	%r15, 0(%r15)
	AGHI	%r15,-160

	LARL	%r14, __ext_int_handler
	BALR	%r14, %r14		# call the real handler

	#
	# NOTE: we may never return from the C-interrupt handler if the
	# interrupt was caused by the timer clock, in which case the
	# scheduler kicks in, and gives control to someone else
	#

	# restore regs
	LMG	%r0,%r15,0x200(%r0)

	LPSWE	0x130			# go back to what we interrupted


#
# Supervisor-Call Interrupt
#
	.align 4
.globl SVC_INT
	.type	SVC_INT, @function
SVC_INT:
	# save regs
	STMG	%r0,%r15,0x200(%r0)

	# set up stack
	LARL	%r15, int_stack_ptr
	LG	%r15, 0(%r15)
	AGHI	%r15,-160

	# find the right handler
	LARL	%r14, svc_table		# table address
	LGH	%r2, 0x8a		# interruption code
	SLL	%r2, 3
					# byte offset into table
	LG	%r14, 0(%r2,%r14)	# load value from table

	BALR	%r14, %r14		# call the real handler

	# restore regs
	LMG	%r0,%r15,0x200(%r0)

	LPSWE	0x140			# go back to what we interrupted


#
# Program Interrupt
#
	.align 4
.globl PGM_INT
	.type	PGM_INT, @function
PGM_INT:
	# save regs
	STMG	%r0,%r15,0x200(%r0)

	# set up stack
	LARL	%r15, int_stack_ptr
	LG	%r15, 0(%r15)
	AGHI	%r15,-160

	LARL	%r14, __pgm_int_handler
	BALR	%r14, %r14		# call the real handler

	#
	# NOTE: we may never return from the C-interrupt handler
	#

	# restore regs
	LMG	%r0,%r15,0x200(%r0)

	LPSWE	0x150			# go back to what we interrupted


#
# Machine Check Interrupt
#
	.align 4
.globl MCH_INT
	.type	MCH_INT, @function
MCH_INT:
	# save regs
	STMG	%r0,%r15,0x200(%r0)

	# set up stack
	LARL	%r15, int_stack_ptr
	LG	%r15, 0(%r15)
	AGHI	%r15,-160

	LARL	%r14, __mch_int_handler
	BALR	%r14, %r14		# call the real handler

	#
	# NOTE: we may never return from the C-interrupt handler
	#

	# restore regs
	LMG	%r0,%r15,0x200(%r0)

	LPSWE	0x160			# go back to what we interrupted
