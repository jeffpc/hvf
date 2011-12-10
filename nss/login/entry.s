/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

.globl START
	.type	START, @function
START:
	# set up the stack
	larl	%r15,STACK

	# jump to C
	larl	%r14,start
	basr	%r14,%r14

.data
.globl STACK
STACK:
	.byte 0
