.text
	.align	4
.globl IO_INT
	.type	IO_INT, @function
IO_INT:
	# save regs
	STMG	%r0,%r15,0x200(%r0)

	# set up stack
	LARL	%r15, io_stack_ptr
	LG	%r15, 0(%r15)
	AGHI	%r15,-160

	LARL	%r14, __io_int_handler
	BALR	%r14, %r14		# call the real handler

	# restore regs
	LMG	%r0,%r15,0x200(%r0)

	LPSWE	0x170			# go back to what we interrupted
