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

