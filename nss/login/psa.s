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

.data
.org 0x120
.globl psw_rst_old
psw_rst_old:

.org 0x130
.globl psw_ext_old
psw_ext_old:

.org 0x140
.globl psw_svc_old
psw_svc_old:

.org 0x150
.globl psw_prg_old
psw_prg_old:

.org 0x160
.globl psw_mch_old
psw_mch_old:

.org 0x170
.globl psw_io_old
psw_io_old:

.org 0x1a0
.globl psw_rst_new
psw_rst_new:

.org 0x1b0
.globl psw_ext_new
psw_ext_new:

.org 0x1c0
.globl psw_svc_new
psw_svc_new:

.org 0x1d0
.globl psw_prg_new
psw_prg_new:

.org 0x1e0
.globl psw_mch_new
psw_mch_new:

.org 0x1f0
.globl psw_io_new
psw_io_new:

