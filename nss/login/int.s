/*
 * Copyright (c) 2007-2011 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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
