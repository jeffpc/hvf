#ifndef __IO_H
#define __IO_H

extern void init_io_int(void);
extern void wait_for_io_int();

extern u32 find_dev(int devnum);

#endif
