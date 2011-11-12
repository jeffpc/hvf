#
# Copyright (c) 2007-2011 Josef 'Jeff' Sipek
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

	# r15 = 0x100000
	#     = (1 << 20)
	#
	SGR	%r15, %r15
	LGHI	%r15, 0x1
	SLLG	%r15, %r15, 20
	AGHI	%r15, -160

	BRC	15,load_nucleus
