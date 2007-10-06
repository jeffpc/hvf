#
# Copyright (c) 2007 Josef 'Jeff' Sipek
#

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

	# r15 = 0x800000
	#     = (8 << 20)
	#
	SR	%r15, %r15
	LHI	%r15, 0x8
	SLL	%r15, 20
	AHI	%r15, -160

#
# Padding to make the entire file 0x20 bytes
#
	BCR	0, %r7
	BCR	0, %r7
