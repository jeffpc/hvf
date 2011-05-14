/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#ifndef __ARCH_H
#define __ARCH_H

struct psw {
	u8 _zero0:1,
	   r:1,			/* PER Mask (R)			*/
	   _zero1:3,
	   t:1,			/* DAT Mode (T)			*/
	   io:1,		/* I/O Mask (IO)		*/
	   ex:1;		/* External Mask (EX)		*/

	u8 key:4,		/* Key				*/
	   fmt:1,		/* 1 on 390, 0 on z		*/
	   m:1,			/* Machine-Check Mask (M)	*/
	   w:1,			/* Wait State (W)		*/
	   p:1;			/* Problem State (P)		*/

	u8 as:2,		/* Address-Space Control (AS)	*/
	   cc:2,		/* Condition Code (CC)		*/
	   prog_mask:4;		/* Program Mask			*/

	u8 _zero2:7,
	   ea:1;		/* Extended Addressing (EA)	*/

	u32 ba:1,		/* Basic Addressing (BA)	*/
	    ptr31:31;

	u64 ptr;
};

#endif
