/*
 * (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

	#
	# We are guaranteed the following:
	#
	# 1) we are located at 16M
	# 2) we are running in ESA/390
	# 3) we are in 31-bit addressing mode
	# 4) the guest registers are saved at GUEST_IPL_REGSAVE
	# 5) R1 contains the subchannel #
	# 6) R2 contains the device #
	#
	# Register usage:
	#
	#   R1  = subchannel number
	#   R2  = device number
	#   R3  = temp
	#   R12 = base for this code
	#

	# Nice register names
.equ	r0,0
.equ	r1,1
.equ	r2,2
.equ	r3,3
.equ	r12,12
.equ	r15,15

	# new IO interruption PSW address
.equ	PSANEWIO,120

###############################################################################
# Code begins here                                                            #
###############################################################################
.text

.globl START
	.type	START, @function
START:
	LARL	r12,START

	STSCH	SCHIB(r12)			# get the SCHIB
	BNZ	ERROR(r12)

	OI	SCHIB+5(r12),0x80		# enable the subchannel

	MSCH	SCHIB(r12)			# load up the SCHIB
	BNZ	ERROR(r12)

	# got a functioning subchannel, let's set up the new IO psw

	MVC	PSANEWIO(8,r0),IOPSW(r12)

	L	r3,PSANEWIO+4(r0,r0)
	LA	r3,IO(r3,r12)			# calculate the address of
	ST	r3,PSANEWIO+4(r0,r0)		# the IO handler

	# let's set up a CCW + an ORB, start the IO, and wait

	MVC	0(8,r0),INITCCW(r12)		# set up the initial CCW
	OI	ORB+6(r12),0xff			# set the LPM in the ORB

	SSCH	ORB(r12)			# start the IO
	BNZ	ERROR(r12)

	LPSW	ENABLEDWAIT(r12)		# wait for IO interruptions

.equ	IO,(.-START)
	# this is the IO interrupt handler

	B	DONE(r12)

.equ	ERROR,(.-START)
	# an error occured, return an error

	LM	r0,r15,REGSAVE(r12)
	DIAG	r0,r0,1				# return to hypervisor

.equ	DONE,(.-START)
	# we were successful!

	LM	r0,r15,REGSAVE(r12)
	DIAG	r0,r0,0				# return to hypervisor

###############################################################################
# Data begins here                                                            #
###############################################################################
.equ	DISABLEDWAIT,(.-START)
	.long	0x000c0000
	.long	0x80000000			# disabled wait PSW; signal
						# return to hypervisor
.equ	ENABLEDWAIT,(.-START)
	.long	0x020c0000
	.long	0x80000000			# enabled wait PSW; wait for IO

.equ	IOPSW,(.-START)
	.long	0x00080000
	.long	0x80000000			# new IO psw

.equ	INITCCW,(.-START)
	.long	0x02000000
	.long	0x60000018			# fmt0, read 24 bytes to addr 0

	.align 4
.equ	ORB,(.-START)
	.skip	(8*4),0				# the ORB is BIG

	.align 4
.equ	SCHIB,(.-START)
	.skip	(13*4),0			# the SCHIB is BIG

	.align 4
.globl GUEST_IPL_REGSAVE
	.type	GUEST_IPL_REGSAVE, @function
GUEST_IPL_REGSAVE:
.equ	REGSAVE,(.-START)			# register save area
