#include <io.h>
#include <cons.h>

void start()
{
	init_io_int();
	init_console(0x0009);

	putline("Hello\n", 6);

	for(;;)
		;
}
