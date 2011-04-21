/*
 * Copyright (c) 2011 Josef 'Jeff' Sipek
 */
#include "loader.h"

static char *top;

void init_malloc(void *ptr)
{
	top = ptr;
}

void *malloc(u32 size)
{
	char *t;

	t = top;
	top += size;

	return t;
}
