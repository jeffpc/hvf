.text

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
	LE	%r2, 0x8a		# interruption code
	SLL	%r2, .svc_num_to_offset_shift
					# byte offset into table
	LG	%r14, 0(%r14,%r2)	# load value from table

	BALR	%r14, %r14		# call the real handler

	# restore regs
	LMG	%r0,%r15,0x200(%r0)

	LPSWE	0x140			# go back to what we interrupted

	.align 4
.svc_num_to_offset_shift:	.word 3

