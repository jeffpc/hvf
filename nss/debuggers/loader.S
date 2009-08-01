#
# Copyright (c) 2009 Josef 'Jeff' Sipek
#

#
# At this point, the machine is running in ESA390 mode. Let's load a new
# PSW, making it switch to 64-bit mode
#

.long 0x00080000
.long loader

loader:
	# switch to 64-bit mode
	#
	# Signal Processor
	#   Order 0x12: Set Architecture
	#   R1 bits 56-63 = 0x01 (switch all CPUs to z/Arch)
	XR	%r1, %r1
	LHI	%r1, 0x1	# switch all to z/Arch
	XR	%r3, %r3	# CPU Address (CPU0000)
	SIGP	%r1, %r3, 0x12	# Signal, order 0x12
	SAM64
	# On error:
	#   Bit 55 = 1, cc1 (inval param)
	#   Bit 54 = 1, cc1 (incorrect state)

	# FIXME: check for errors?

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

	LG	%r1, subch
	STG	%r1, 0x200(%r0)

	LG	%r1, addr
	BR	%r1

subch:
.8byte 0x10005
addr:
.8byte DEBUGADDR

IOSVC:
	lg	%r1,0x200(%r0)
	tsch	IRB
	lpswe	0x170(%r0)

.align 8
IRB:
.skip 24*4

.org 0x1d0
.long 0x00020000
.long 0x00000000
.8byte 0

.org 0x1f0
.long 0x00000001
.long 0x80000000
.8byte IOSVC
