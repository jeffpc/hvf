#
# Copyright (c) 2007-2009 Josef 'Jeff' Sipek
#

.globl MAIN
	.type	MAIN, @function
MAIN:
#
# At this point, the machine is running in ESA390 mode. Let's load a new
# PSW, making it switch to 64-bit mode
#

	# switch to 64-bit mode
	#
	# Signal Processor
	#   Order 0x12: Set Architecture
	#   R1 bits 56-63 = 0x01 (switch all CPUs to z/Arch)
	SR	%r1, %r1
	LHI	%r1, 0x1	# switch all to z/Arch
	SR	%r3, %r3	# CPU Address (CPU0000)
	SIGP	%r1, %r3, 0x12	# Signal, order 0x12
	SAM64
	# On error:
	#   Bit 55 = 1, cc1 (inval param)
	#   Bit 54 = 1, cc1 (incorrect state)

	# FIXME: check for errors?

#
# At this point, we should be in 64-bit mode
#

	#
	# It is unfortunate that the below code is required.
	#
	# Let's set the stack pointer to make gcc happy
	#
	# A standard stack frame is 160 bytes.
	#
	# NOTE: Once this thread of execution turns into the idle thread,
	# the stack can be reused for something else. Since it's allocated
	# in the 128th processor's PSA, we have to make sure that it won't
	# get initialized until after the idle thread kicks in.
	#

	# r15 = 0x100000
	#     = (1 << 20)
	#
	SR	%r15, %r15
	LHI	%r15, 0x1
	SLL	%r15, 20
	AHI	%r15, -160

	BRC	15,load_nucleus
