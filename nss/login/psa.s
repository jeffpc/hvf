/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
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

