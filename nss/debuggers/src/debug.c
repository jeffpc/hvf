u64 dis_wait_psw[2] = {
	//0x0002000180000000ull,
	0x0202000180000000ull,
	0x0000000000000000ull,
};

static inline void lpswe(u64 *psw)
{
	asm volatile(
		"lpswe	0(%0)"
	: /* output */
	: /* input */
	  "a" (psw)
	);
}

void full_draw();

void start()
{
	u64 cur_psw[2] = {
		0x0200000180000000ull,
		0x0000000000000000ull,
	};

	u32 SCHIB[0];

	u64 cr6;

	/* enable all I/O interrupt classes */
	asm volatile(
		"stctg	6,6,%0\n"	/* get cr6 */
		"oi	%1,0xff\n"	/* enable all */
		"lctlg	6,6,%0\n"	/* reload cr6 */
	: /* output */
	: /* input */
	  "m" (cr6),
	  "m" (*(u64*) (((u8*)&cr6) + 4))
	);

	asm volatile(
		"	larl	%%r1,0f\n"
		"	stg	%%r1,%2\n"
		"	lpswe	%3\n"
		"0:\n"
		"	lg	%%r1, 0(%1)\n"
		"	stsch	%0\n"
		"	oi	5+%0,0x80\n"
		"	msch	%0\n"
	: /* out */
	: /* in */
	  "m" (SCHIB[0]),
	  "a" ((void*) 0x200),
	  "m" (cur_psw[1]),
	  "m" (cur_psw[0])
	: /* clob */
	  "cc", "r1"
	);

	full_draw();

	lpswe(dis_wait_psw);
}

u32 CCW[2];
u32 ORB[8];

void write(char *ptr, int len)
{
	CCW[0] = 0x05000000 | (u32) (u64) ptr;
	CCW[1] = 0x20000000 | (u32) len;

	__builtin_memset(ORB, 0, 8*4);

	asm volatile(
		"oi	6(%0),0xff\n"
		"st	%2, 8(%0)\n"
		"lg	%%r1, 0(%1)\n"
		"ssch	0(%0)\n"
	: /* out */
	: /* in */
	  "a" (ORB),
	  "a" ((void*)0x200),
	  "a" (CCW)
	: /* clob */
	  "cc", "r1"
	);
}

/******************************************************************************/
/* 3270 helpers                                                               */
/******************************************************************************/

#define WE	'\x05'

#define SBA	'\x11'
#define IC	'\x13'
#define SF	'\x1d'

int start_field(char *ptr, int prot, int num, int highlight, int sel)
{
	u8 attr = 0xc0;

	if (prot)
		attr |= 0x20;
	if (num)
		attr |= 0x10;

	if (highlight)
		attr |= 0x08;
	else if (sel)
		attr |= 0x04;

	ptr[0] = SF;
	ptr[1] = attr;

	return 2;
}

int insert_cur(char *ptr)
{
	ptr[0] = IC;

	return 1;
}

int set_addr(char *ptr, int r, int c)
{
	ptr[0] = SBA;
	ptr[1] = (r*80+c) / 256;
	ptr[2] = (r*80+c) % 256;

	return 3;
}

/******************************************************************************/
/* stringification helpers                                                    */
/******************************************************************************/

static inline u8 __ebc_dig(u32 d)
{
	if (d < 10)
		return '\xf0' + d;
	return '\xc1' - 10 + d;
}

int place_hex(char *ptr, u64 v, int width)
{
	u64 i;

	for(i=width; i; i -= 4, ptr++)
		*ptr = __ebc_dig((v >> (i-4)) & 0xf);

	return (width/8)*2;
}

int place_int(char *ptr, u64 v, int width)
{
	int i;

	for(ptr+=width-1, i=width; i; i--, ptr--) {
		if (v || i==width) {
			*ptr = __ebc_dig(v % 10);
			v /= 10;
		} else
			*ptr = '\x40';
	}

	return width;
}

/******************************************************************************/
/* useful constant strings                                                    */
/******************************************************************************/

const char* rule =
		"\x4e\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60"
		"\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60"
		"\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60"
		"\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60"
		"\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x60\x4e";

const char *prompt = "\x7e\x7e\x7e\x6e"; /* "===>" */

const char *mode[2] = {
	[0] = "\xe2\x61\xf3\xf9\xf0", /* "S/390" */
	[1] = "\xc5\xe2\xc1\xd4\xc5", /* "ESAME" */
};

const char *as[4] = {
	[0] = "\xd7", /* P */
	[1] = "\xc1", /* A */
	[2] = "\xe2", /* S */
	[3] = "\xc8", /* H */
};

const char *amode[4] = {
	[0] = "\xf2\xf4", /* 24 */
	[1] = "\xf0\xf0", /* ?? */
	[2] = "\xf3\xf1", /* 31 */
	[3] = "\xf6\xf4", /* 64 */
};

/******************************************************************************/
/* the VM state                                                               */
/******************************************************************************/

int S_mode = 1;
u32 S_psw[4];
u64 S_gpr[16];
u64 S_cr[16];
u32 S_ar[16];

/******************************************************************************/
/* the code                                                                   */
/******************************************************************************/

#define BUF_SIZE	(24*80)

char buf[BUF_SIZE];

int place_str(char *ptr, char *str, int len)
{
	__builtin_memcpy(ptr, str, len);
	return len;
}

#define HL_REG(x)	(1 << (x))
#define HL_NONE		0x0000
int place_xrs(char *ptr, u64 *regs, u16 mask)
{
	int len = 0;
	int i;

	if (S_mode) {
		for(i=0; i<16; i++) {
			len += place_int(&ptr[len], i, 2);
			len += start_field(&ptr[len], 0, 0, HL_REG(i) & mask, 0);
			len += place_hex(&ptr[len], regs[i], 64);
			len += start_field(&ptr[len], 1, 0, 0, 1);
		}
	} else {
		for(i=0; i<16; i++) {
			if (i==0)
				len += place_str(&ptr[len], "\x40\x40\xf0\x40", 4);
			if (i==8) {
				len += start_field(&ptr[len], 1, 0, 0, 1);
				len += place_str(&ptr[len], "\x40\x40\x40\x40\x40\xf8\x40", 7);
			}
			len += start_field(&ptr[len], 0, 0, HL_REG(i) & mask, 0);
			len += place_hex(&ptr[len], regs[i] & 0xffffffffull, 32);
		}
		len += start_field(&ptr[len], 1, 0, 0, 1);
		len += place_str(&ptr[len], "\x40\x40\x40", 3);
	}

	return len;
}

void full_draw()
{
	int len = 2;

	S_gpr[1] = 0xcafebabedeadbeef;
	S_gpr[3] = 1980;
	S_gpr[9] = 1985;
	S_cr[0] = 0xdeadbeefcafebabe;

	buf[0] = WE;
	buf[1] = '\xf7'; // FIXME

	len += set_addr(&buf[len], 0, 0);
	len += place_str(&buf[len], (char*) rule, 80);
	len += start_field(&buf[len], 0, 0, 0, 1);
	len += place_hex(&buf[len], S_psw[0], 32);
	len += start_field(&buf[len], 0, 0, 0, 1);
	len += place_hex(&buf[len], S_psw[1], 32);
	if (S_mode) {
		len += start_field(&buf[len], 0, 0, 0, 1);
		len += place_hex(&buf[len], S_psw[2], 32);
		len += start_field(&buf[len], 0, 0, 0, 1);
		len += place_hex(&buf[len], S_psw[3], 32);
	}
	len += start_field(&buf[len], 1, 0, 0, 0);
	len += place_str(&buf[len], (S_psw[0] & 0x40000000) ? "\xd9" : "\x4b", 1);
	len += place_str(&buf[len], (S_psw[0] & 0x04000000) ? "\xe3" : "\x4b", 1);
	len += place_str(&buf[len], (S_psw[0] & 0x02000000) ? "\xc9" : "\x4b", 1);
	len += place_str(&buf[len], (S_psw[0] & 0x01000000) ? "\xc5" : "\x4b", 1);
	len += place_str(&buf[len], (S_psw[0] & 0x00040000) ? "\xd4" : "\x4b", 1);
	len += place_str(&buf[len], (S_psw[0] & 0x00020000) ? "\xe5" : "\x4b", 1);
	len += place_str(&buf[len], (S_psw[0] & 0x00010000) ? "\xd7" : "\x4b", 1);
	len += place_str(&buf[len], "\x40\xd2\x85\xa8\x7a", 5);
	len += place_int(&buf[len], (S_psw[0] >> 20) & 0xf, 2);
	len += place_str(&buf[len], "\x40\xc1\xe2\x7a", 4);
	len += place_str(&buf[len],
			 (S_psw[0] & 0x40000000) ?
				 (char*)as[(S_psw[0] & 0x0000c000) >> 14] : "\xd9",
			 1);
	len += place_str(&buf[len],
			 (char*)amode[(S_psw[0] & 1) | ((S_psw[1] & 0x80000000) >> 30)],
			 2);
	len += place_str(&buf[len], "\x40\xc3\xc3\x7a", 4);
	len += place_int(&buf[len], (S_psw[0] >> 12) & 0x3, 1);
	len += set_addr(&buf[len], 1, 75);
	len += place_str(&buf[len], (char*)mode[S_mode], 5);
	len += place_str(&buf[len], (char*)rule, 80);
	len += place_xrs(&buf[len], S_gpr, HL_REG(10));
	len += place_str(&buf[len], (char*)rule, 80);
	len += place_xrs(&buf[len], S_cr, HL_NONE);
	len += place_str(&buf[len], (char*)rule, 80);

	len += set_addr(&buf[len], 23, 0);
	len += start_field(&buf[len], 1, 0, 1, 0);
	len += place_str(&buf[len], (char*)prompt, 5);
	len += start_field(&buf[len], 0, 0, 0, 1);
	len += insert_cur(&buf[len]);
	len += set_addr(&buf[len], 23, 77);
	len += start_field(&buf[len], 1, 0, 0, 0);

	buf[len++] = '\xff';
	buf[len++] = '\xef';

	write(buf, len);
}
