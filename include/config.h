#ifndef __CONFIG_H
#define __CONFIG_H

#define MEMSIZE			(1024*1024*32)

#if MEMSIZE > (1024*1024*1024*2UL)
#error Config with 2+ GB of storage is not supported (need to fix DAT)
#endif

#endif
