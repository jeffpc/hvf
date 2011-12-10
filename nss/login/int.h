/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __INT_H
#define __INT_H

extern void rst_int();
extern void ext_int();
extern void svc_int();
extern void prg_int();
extern void mch_int();
extern void io_int();

extern struct psw psw_rst_old;
extern struct psw psw_ext_old;
extern struct psw psw_svc_old;
extern struct psw psw_prg_old;
extern struct psw psw_mch_old;
extern struct psw psw_io_old;
extern struct psw psw_rst_new;
extern struct psw psw_ext_new;
extern struct psw psw_svc_new;
extern struct psw psw_prg_new;
extern struct psw psw_mch_new;
extern struct psw psw_io_new;

#endif
