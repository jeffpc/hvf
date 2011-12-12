/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <interrupt.h>
#include <channel.h>
#include <sclp.h>

static void get_crw()
{
	struct crw crw;
	int ret;

	do {
		ret = store_crw(&crw);
		if (ret)
			break;

		sclp_msg("%08x:", *(u32*)&crw);
		sclp_msg("RSC: %x", crw.rsc);
		sclp_msg("ERC: %x", crw.erc);
		sclp_msg("ID:  %x", crw.id);

		if (!crw.c)
			break;
	} while(1);
}

void __mch_int_handler(void)
{
	sclp_msg("MCH INTERRUPT!");
	sclp_msg("%016llx: ", *(u64*)MCH_INT_CODE);
	if (MCH_INT_CODE->sd)
		sclp_msg("-> system damage");
	if (MCH_INT_CODE->pd)
		sclp_msg("-> instruction processing damage");
	if (MCH_INT_CODE->sr)
		sclp_msg("-> system recovery");
	if (MCH_INT_CODE->cd)
		sclp_msg("-> timing-facility damage");
	if (MCH_INT_CODE->ed)
		sclp_msg("-> external damage");
	if (MCH_INT_CODE->dg)
		sclp_msg("-> degradation");
	if (MCH_INT_CODE->w)
		sclp_msg("-> warning");
	if (MCH_INT_CODE->cp)
		sclp_msg("-> channel report pending");
	if (MCH_INT_CODE->sp)
		sclp_msg("-> service-processor damage");
	if (MCH_INT_CODE->ck)
		sclp_msg("-> channel-subsystem damage");

	if (MCH_INT_CODE->sd)
		BUG();

	if (MCH_INT_CODE->ck)
		BUG();

	if (MCH_INT_CODE->cp)
		get_crw();
}
