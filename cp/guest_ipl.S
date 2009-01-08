	#
	# We are guaranteed the following:
	#
	# 1) we are located above 16M
	# 2) we are located below 2G
	# 3) we are running in ESA/390
	# 4) we are in 31-bit addressing mode
	# 5) the guest registers are saved at GUEST_IPL_REGSAVE
	# 6) R1 contains the subchannel #
	# 7) R2 contains the device #
	# 8) R12 contains the base address
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

.globl GUEST_IPL_CODE
	.type	GUEST_IPL_CODE, @function
GUEST_IPL_CODE:
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

	MVC	INITCCW(8,r12),0(r0)		# set up the initial CCW
	OI	ORB+6(r12),0xff			# set the LPM in the ORB

	SSCH	ORB(r12)			# start the IO
	BNZ	ERROR(r12)

	LPSW	ENABLEDWAIT(r12)		# wait for IO interruptions

.equ	IO,(.-GUEST_IPL_CODE)
	# this is the IO interrupt handler

	B	DONE(r12)

.equ	ERROR,(.-GUEST_IPL_CODE)
	# an error occured, return an error

	LM	r0,r15,REGSAVE(r12)
	DIAG	r0,r0,1				# return to hypervisor

.equ	DONE,(.-GUEST_IPL_CODE)
	# we were successful!

	LM	r0,r15,REGSAVE(r12)
	DIAG	r0,r0,0				# return to hypervisor

###############################################################################
# Data begins here                                                            #
###############################################################################

.equ	DISABLEDWAIT,(.-GUEST_IPL_CODE)
	.long	0x000c0000
	.long	0x80000000			# disabled wait PSW; signal
						# return to hypervisor
.equ	ENABLEDWAIT,(.-GUEST_IPL_CODE)
	.long	0x020c0000
	.long	0x80000000			# enabled wait PSW; wait for IO

.equ	IOPSW,(.-GUEST_IPL_CODE)
	.long	0x00080000
	.long	0x80000000			# new IO psw

.equ	INITCCW,(.-GUEST_IPL_CODE)
	.long	0x02000000
	.long	0x60000018			# fmt0, read 24 bytes to addr 0

	.align 4
.equ	ORB,(.-GUEST_IPL_CODE)
	.skip	(8*4),0				# the ORB is BIG

	.align 4
.equ	SCHIB,(.-GUEST_IPL_CODE)
	.skip	(13*4),0			# the SCHIB is BIG

	.align 4
.globl GUEST_IPL_REGSAVE
	.type	GUEST_IPL_REGSAVE, @function
GUEST_IPL_REGSAVE:
.equ	REGSAVE,(.-GUEST_IPL_CODE)		# register save area
