/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __SCLP_H
#define __SCLP_H

extern void sclp_msg(const char *fmt, ...)
        __attribute__ ((format (printf, 1, 2)));

#endif
