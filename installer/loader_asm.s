#
# Copyright (c) 2007-2011 Josef 'Jeff' Sipek
#

#include "loader.h"

.text
	.align	4
.globl __wait_for_attn
	.type	__wait_for_attn, @function
__wait_for_attn:
	LARL	%r4,WAITPSW		# wait for the interrupt
	LPSWE	0(%r4)

# int __do_io(u32 sch);
	.align	4
.globl __do_io
	.type	__do_io, @function
__do_io:
	LGR	%r1,%r2			# load subsystem ID

	LA	%r2,0xfff		# error return code

	LARL	%r4,ORB
	SSCH	0(%r4)			# issue IO
	BCR	7, %r14			# return on error

	LARL	%r4,WAITPSW		# wait for the interrupt
	LPSWE	0(%r4)

#
# The IO interrupt handler; it's very much like a continuation of __do_io
#
.globl IOHANDLER
IOHANDLER:
	# is this for us?
	LARL	%r1,MAGICVAL
	L	%r1,0(%r1)
	C	%r1,0xbc
	BRC	7,IONOTDONE

	# it is!

	LLGF	%r1,0xb8		# load subsystem ID

	LARL	%r4,IRB
	TSCH	0(%r4)

	# check the Channel Status for != 0
	XGR	%r2,%r2
	IC	%r2,9(%r4)
	NILL	%r2,0xbf		# get rid of SLI
	LTR	%r2,%r2
	BCR	7,%r14			# error, let's bail

	# check the Device Status
	XGR	%r1,%r1
	IC	%r1,8(%r4)

	# unit check, unit except?
	LR	%r2,%r1
	NILL	%r2,0x03
	BCR	4,%r14			# error return

	# attention, DE?
	XGR	%r2,%r2
	NILL	%r1,0x84
	BCR	4,%r14			# ok return

IONOTDONE:
	LPSWE	0x170

#
# The PGM interrupt handler
#
.globl PGMHANDLER
PGMHANDLER:
	STMG	%r1,%r3,0x200

	# r3 = 0x80000000
	XGR	%r3, %r3
	LHI	%r3, 0x8
	SLL	%r3, 20

	# is ILC == 3?
	LGH	%r2,0x8C
	CGHI	%r2,0x0006
	BNE	ERR(%r3)

	# grab the old PSW address, subtract length of TPROT, and compare it
	# with the TPROT opcode (0xe501)
	LG	%r1,0x158
	AGHI	%r1,-6
	LLGH	%r2,TPROTOP(%r3)
	LLGH	%r1,0(%r1)
	CGR	%r2,%r1
	BNE	ERR(%r3)

	# set CC=3
	OI	0x152,0x30

	LMG	%r1,%r3,0x200

	LPSWE	0x150

ERR:
.byte	0x00, 0x00


#
# Useful data
#
.data
.globl TPROTOP
TPROTOP:
.byte	0xe5, 0x01

	.align 8
.globl IOPSW
IOPSW:
	.byte	0x00
		#   bits  value   name                        desc
		#      0      0   <zero>
		#      1      0   PER Mask (R)                disabled
		#    2-4      0   <zero>
		#      5      0   DAT Mode (T)                disabled
		#      6      0   I/O Mask (IO)               enabled
		#      7      0   External Mask (EX)          disabled

	.byte	0x00
		#   bits  value   name                        desc
		#   8-11      0   Key
		#     12      0   <one>
		#     13      0   Machine-Check Mask (M)      disabled
		#     14      0   Wait State (W)              executing
		#     15      0   Problem State (P)           supervisor state

	.byte	0x00
		#   bits  value   name                        desc
		#  16-17      0   Address-Space Control (AS)  disabled
		#  18-19      0   Condition Code (CC)
		#  20-23      0   Program Mask                exceptions disabled

	.byte	0x01
		#   bits  value   name                        desc
		#  24-30      0   <zero>
		#     31      1   Extended Addressing (EA)    EA + BA = 64 mode

	.byte	0x80, 0x00, 0x00, 0x00
	.byte	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		#   bits  value   name                        desc
		#     32      1   Basic Addressing (BA)       BA = 31, !BA = 24
		#  33-63      0   <zero>
		# 64-127   addr   Instruction Address         Address to exec

.globl WAITPSW
WAITPSW:
	.byte	0x02
		#   bits  value   name                        desc
		#      0      0   <zero>
		#      1      0   PER Mask (R)                disabled
		#    2-4      0   <zero>
		#      5      0   DAT Mode (T)                disabled
		#      6      1   I/O Mask (IO)               enabled
		#      7      0   External Mask (EX)          disabled

	.byte	0x02
		#   bits  value   name                        desc
		#   8-11      0   Key
		#     12      0   <zero>
		#     13      0   Machine-Check Mask (M)      disabled
		#     14      1   Wait State (W)              not executing
		#     15      0   Problem State (P)           supervisor state

	.byte	0x00
		#   bits  value   name                        desc
		#  16-17      0   Address-Space Control (AS)  disabled
		#  18-19      0   Condition Code (CC)
		#  20-23      0   Program Mask                exceptions disabled

	.byte	0x01
		#   bits  value   name                        desc
		#  24-30      0   <zero>
		#     31      1   Extended Addressing (EA)    EA + BA = 64 mode

	.byte	0x80, 0x00, 0x00, 0x00
	.byte	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		#   bits  value   name                        desc
		#     32      1   Basic Addressing (BA)       BA = 31, !BA = 24
		#  33-63      0   <zero>
		# 64-127   addr   Instruction Address         Address to exec

.globl IRB
IRB:
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00

	.align 4
.globl ORB
ORB:
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00
	.8byte 0x00

.globl MAGICVAL
MAGICVAL:
	.4byte 0x12345678
