.text
	.align 4
.globl _start
	.type _start, @function
_start:
#
# At this point, we should be in 64-bit mode
#
	# r15 = 0x100000
	#     = (1 << 20)
	#
	XR	%r15, %r15
	LHI	%r15, 0x3
	SLL	%r15, 12
	AHI	%r15, -160

	BRAS	%r14, start

	SR	%r1, %r1
	SR	%r3, %r3
	SIGP	%r1, %r3, 0x05
