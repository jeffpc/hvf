/*
 * Copyright (c) 2007-2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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

	L	%r1, 0xb8		# load subsystem ID
	
	SSCH	ORB(%r4)		# issue IO

/*
7) Enable the PSW for I/O interrupts and go into wait state (you need bits 6, 12 & 14 set to 1 in the PSW : X'020A000000000000' is a good example)
*/
	LPSWE	WAITPSW(%r4)

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

	TSCH	irb(%r4)

/*
FIXME: we should do more checking!

11) If Unit check or Channel Status|=0 : An I/O error occurred and act accordingly
12) If unit exception : End of media (for tape & cards) and act accordingly
13) If device end : I/O Completed.. Perform post I/O stuff (like advancing your pointers) and back to step 3
*/

	# unit check? (end of media?)
	L	%r1,irb+5(%r4)
	LA	%r0,0x02
	NR	%r0,%r1
	LA	%r2,1			# return 1 - end of medium
	BCR	4,%r14			# unit chk => return

	# check the SCSW.. If CE Only : LPSW Old I/O PSW
	LA	%r0,0x04
	NR	%r0,%r1
	LA	%r2,0			# means IO done
	BCR	4,%r14			# device end => return

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

.globl WAITPSW
	.align 8
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

.globl ADDRMASK
ADDRMASK:
	.4byte 0x7fffffff

.globl MAGICVAL
MAGICVAL:
	.4byte 0x12345678
