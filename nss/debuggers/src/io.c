#include <channel.h>
#include <arch.h>
#include <die.h>
#include <string.h>

u32 find_dev(int devnum)
{
	struct schib schib;
	u32 sch;

	for(sch=0x10000; sch<=0x1ffff; sch++) {
		if (store_sch(sch, &schib))
			continue;

		if (!schib.pmcw.v)
			continue;

		if (schib.pmcw.dev_num != devnum)
			continue;

		if (!schib.pmcw.e) {
			schib.pmcw.e = 1;

			if (modify_sch(sch, &schib))
				continue;
		}

		return sch;
	}

	return 0;
}

extern void IOINT(void);

void init_io_int()
{
	struct psw psw;

	// set up the IO interrupt handler
	psw.ea  = 1;
	psw.ba  = 1;
	psw.ptr = (u64) &IOINT;

	memcpy(((void*) 0x1f0), &psw, sizeof(struct psw));
}

void wait_for_io_int()
{
	struct psw psw;
	u8 devst;

        memset(&psw, 0, sizeof(struct psw));
        psw.io  = 1;
        psw.ea  = 1;
        psw.ba  = 1;
	psw.w   = 1;

        asm volatile(
                "       larl    %%r1,0f\n"
                "       stg     %%r1,%0\n"
                "       lpswe   %1\n"
                "0:\n"
        : /* output */
          "=m" (psw.ptr)
        : /* input */
          "m" (psw)
        : /* clobbered */
          "r1", "r2"
        );

	devst = *((u8*) 0x210);

	if (devst & 0x84)
		return;

	die();
}
