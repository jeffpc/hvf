/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

.text

	.align	4
.globl die
	.type	die, @function
die:
	larl	%r1,disabled_wait
	lpswe	0(%r1)
