#include <ebcdic.h>
#include <channel.h>
#include <string.h>

#include <io.h>
#include <die.h>

#define CON_LEN 132

static u32 consch;

int putline(char *buf, int len)
{
	char data[CON_LEN];
	struct orb orb;
	struct ccw ccw;

	if ((len <= 0) || (len > CON_LEN))
		return -1;

	memcpy(data, buf, len);
	ascii2ebcdic((u8*) data, len);

	ccw.cmd   = 0x01;
	ccw.flags = 0;
	ccw.count = len;
	ccw.addr  = (u32) (u64) data;

	memset(&orb, 0, sizeof(struct orb));
	orb.lpm   = 0xff;
	orb.addr  = (u32) (u64) &ccw;
	orb.f     = 1;

	if (start_sch(consch, &orb))
		die();

	wait_for_io_int();

	return len;
}

int getline(char *buf, int len)
{
	struct orb orb;
	struct ccw ccw;
	int i;

	if ((len <= 0) || (len > CON_LEN))
		return -1;

	wait_for_io_int();

	memset(buf, 0, len);
	ccw.cmd   = 0x0a;
	ccw.flags = 0x20;
	ccw.count = len;
	ccw.addr  = (u32) (u64) buf;

	memset(&orb, 0, sizeof(struct orb));
	orb.lpm   = 0xff;
	orb.addr  = (u32) (u64) &ccw;
	orb.f     = 1;

	if (start_sch(consch, &orb))
		die();

	wait_for_io_int();

	for(i=0; i<len; i++)
		if (!buf[i])
			break;

	if (i)
		ebcdic2ascii((u8*) buf, i);

	return i;
}

void init_console(u16 devnum)
{
	u32 sch;

	sch = find_dev(devnum);

	if (!sch)
		die();

	consch = sch;
}
