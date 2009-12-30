#
# Copyright (c) 2007 Josef 'Jeff' Sipek
#

#include "loader.h"

.text
	.align	4
.globl __do_io
	.type	__do_io, @function
__do_io:
	#
	# r4 = 0x80000000
	#
	XGR	%r4, %r4
	LHI	%r4, 0x8
	SLL	%r4, 20
	#
	# r14 = r14 & 0x7fffffff don't ask, it's strangely retarded
	#
	L	%r1,ADDRMASK(%r4)
	NR	%r14, %r1  # mask out the bit

	# set up the interrupt handler
	MVC	0x1f0(16),IOPSW(%r4)	# SET NEW IO PSW
	LA	%r1,IOHANDLER(%r4)	# GET HANDLER ADDRESS
	STG	%r1,0x1f0+8		# SAVE IN NEW PSW

	L	%r1, 0xb8		# load subsystem ID
	
	SSCH	ORB(%r4)		# issue IO

	# Load Control Register 6 with I/O interrupt subclass mask
	STCTG	6,6,TMPVAR(%r4)		# GET CR6
	OI	TMPVAR+4(%r4),0xFF	# enable all
	LCTLG	6,6,TMPVAR(%r4)		# RELOAD MODIFIED CR6

/*
7) Enable the PSW for I/O interrupts and go into wait state (you need bits 6, 12 & 14 set to 1 in the PSW : X'020A000000000000' is a good example)
*/
	LA	%r3, RETURN(%r4)	# interrupt return address
	#STG	%r1, WAITPSW+8(%r4)	

	LPSWE	WAITPSW(%r4)

RETURN:
	SR	%r2, %r2	# return 0
	BR	%r14		# Done, let's go back to C!

#
# The IO interrupt handler
#
.globl IOHANDLER
IOHANDLER:
	# is this for us?
	L	%r1, MAGICVAL(%r4)
	C	%r1, 0xbc
	BNE	IONOTDONE(%r4)

	# it is!

	L	%r1, 0xb8		# load subsystem ID

	TSCH	IRB(%r4)

	# check the SCSW.. If CE Only : LPSW Old I/O PSW
	L	%r1, IRB+5(%r4)
	NILL	%r1, 0x04
	BC	8, IONOTDONE(%r4)	# not device end => LPSWE

/*
FIXME: we should do more checking!

11) If Unit check or Channel Status|=0 : An I/O error occurred and act accordingly
12) If unit exception : End of media (for tape & cards) and act accordingly
13) If device end : I/O Completed.. Perform post I/O stuff (like advancing your pointers) and back to step 3
*/

	# return to the "caller"
	BR	%r3

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
	BNE	ERR

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

	.byte	0x80
	.byte	0x00
	.byte	0x00
	.byte	0x00
	.byte	0x00
	.byte	0x00
	.byte	0x00
	.byte	0x00
	.byte	0x00
	.byte	0x00
	.byte	0x00
	.byte	0x00
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

	.byte	0x80
	.byte	0x00
	.byte	0x00
	.byte	0x00
	.byte	0x00
	.byte	0x00
	.byte	0x00
	.byte	0x00
	.byte	0x00
	.byte	0x00
	.byte	0x00
	.byte	0x00
		#   bits  value   name                        desc
		#     32      1   Basic Addressing (BA)       BA = 31, !BA = 24
		#  33-63      0   <zero>
		# 64-127   addr   Instruction Address         Address to exec

.globl TMPVAR
TMPVAR:
	.8byte 0x0

.globl ADDRMASK
ADDRMASK:
	.4byte 0x7fffffff

.globl MAGICVAL
MAGICVAL:
	.4byte 0x12345678

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

