/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

.text

/* we should never get a reset interrupt */
	.align	4
.globl rst_int
	.type	rst_int, @function
rst_int:
	larl	%r1,disabled_wait
	lpswe	0(%r1)

/* we should never get an external interrupt */
	.align	4
.globl ext_int
	.type	ext_int, @function
ext_int:
	larl	%r1,disabled_wait
	lpswe	0(%r1)

/* we should never get a svc interrupt */
	.align	4
.globl svc_int
	.type	svc_int, @function
svc_int:
	larl	%r1,disabled_wait
	lpswe	0(%r1)
