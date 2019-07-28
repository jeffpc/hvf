/*
 * Copyright (c) 2011-2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static char *deck_head =
"#\n"
"# NOTE: This file was automatically generated by " __FILE__ "\n"
"#\n"
"# During the system IPL, 24 bytes are read from the device.\n"
"#\n"
"# NOTE: zArch IPLs in ESA/390 mode.\n"
"#\n"
"\n"
".text\n"
"	.align	4\n"
".globl START\n"
"	.type	START, @function\n"
"START:\n";

static char *card_head =
"\n"
"#######################################################################\n"
"# CARD %-3d                                                            #\n"
"#######################################################################\n";

static char *psw_head =
"#\n"
"# Bytes 0-7 contain PSW to be loaded after IO operation completes\n"
"#\n";

static char *psw =
"	.byte	0x%c%c\n"
"		#   bits  value   name                        desc\n"
"		#      0      0   <zero>\n"
"		#      1      0   PER Mask (R)                disabled\n"
"		#    2-4      0   <zero>\n"
"		#      5      0   DAT Mode (T)                disabled\n"
"		#      6      0   I/O Mask (IO)               disabled\n"
"		#      7      0   External Mask (EX)          disabled\n"
"\n"
"	.byte	0x%c%c\n"
"		#   bits  value   name                        desc\n"
"		#   8-11      0   Key\n"
"		#     12      1   <one>\n"
"		#     13      0   Machine-Check Mask (M)      disabled\n"
"		#     14      0   Wait State (W)              executing\n"
"		#     15      0   Problem State (P)           supervisor state\n"
"\n"
"	.byte	0x%c%c\n"
"		#   bits  value   name                        desc\n"
"		#  16-17      0   Address-Space Control (AS)  disabled\n"
"		#  18-19      0   Condition Code (CC)\n"
"		#  20-23      0   Program Mask                exceptions disabled\n"
"\n"
"	.byte	0x%c%c\n"
"		#   bits  value   name                        desc\n"
"		#  24-30      0   <zero>\n"
"		#     31      0   Extended Addressing (EA)    ! 64 mode\n"
"\n"
"	.byte	0x%c%c	# bits 32-39\n"
"	.byte	0x%c%c	# bits 40-47\n"
"	.byte	0x%c%c	# bits 48-55\n"
"	.byte	0x%c%c	# bits 56-63\n"
"		#   bits  value   name                        desc\n"
"		#     32      1   Basic Addressing (BA)       BA = 31, !BA = 24\n"
"		#  33-63   addr   Instruction Address         Address to exec\n";

static char *psw_tail =
"\n"
"#\n"
"# The remaining 16 bytes should contain CCW to read data from device\n"
"#\n"
"\n"
"# CCW format-0:\n"
"#   bits  name\n"
"#    0-7  Cmd Code\n"
"#   8-31  Data Address\n"
"#     32  Chain-Data (CD)\n"
"#     33  Chain-Command (CC)\n"
"#     34  Sup.-Len.-Inditcation (SLI)\n"
"#     35  Skip (SKP)\n"
"#     36  Prog.-Contr.-Inter. (PCI)\n"
"#     37  Indir.-Data-Addr. (IDA)\n"
"#     38  Suspend (S)\n"
"#     39  Modified I.D.A. (MIDA)\n"
"#  40-47  <ignored>\n"
"#  48-63  number of bytes to read\n"
"\n"
"# CP-CCW 1\n"
"	# READ 80 bytes of CCWs to 0x%06X\n"
"	.byte	0x02, 0x%02X, 0x%02X, 0x%02X\n"
"	.byte	0x%02X, 0x00, 0x00, 0x50\n"
"\n"
"# CP-CCW 2\n"
"	# TIC to 0x%06X\n"
"	.byte	0x08, 0x%02X, 0x%02X, 0x%02X\n"
"	.byte	0x00, 0x00, 0x00, 0x00\n";

static char *pad =
"\n"
"#\n"
"# Pad to 80 bytes\n"
"#\n"
"	.byte 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40\n"
"	.byte 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40\n"
"	.byte 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40\n"
"	.byte 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40\n"
"	.byte 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40\n"
"	.byte 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40\n"
"	.byte 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40\n";

static char *cp_ccw =
"\n"
"# CP-CCW %d\n"
"	.byte	0x02, 0x%02X, 0x%02X, 0x%02X\n"
"	.byte	0x40, 0x00, 0x00, 0x50\n";

#define CHAIN		0x40

static char *data_ccw =
"\n"
"# LOADER-CCW %d\n"
"	.byte	0x02, 0x%02X, 0x%02X, 0x%02X\n"
"	.byte	0x%02X, 0x00, 0x00, 0x50\n";

static char *nop_ccw =
"\n"
"# NOP-CCW %d\n"
"	.byte	0x03, 0x00, 0x00, 0x08\n"
"	.byte	0x60, 0x00, 0x00, 0x01\n";

#define ADDR(x)		(((x)[0]<<16) | \
			 ((x)[1]<<8)  | \
			 ((x)[2]))

#define ADDRS(x)	(x)[0], (x)[1], (x)[2]

static void die(char *pgm, char *s)
{
	fprintf(stderr, "Error: %s\n", s);
	fprintf(stderr, "Usage: %s <psw word 1> <psw word 2> <loaded file> "
		"<address> <channel pgm addr>\n", pgm);
	exit(1);
}

static unsigned int get_file_size(const char *fname)
{
	struct stat buf;
	int ret;

	ret = stat(fname, &buf);

	return ret ? 0 : buf.st_size;
}

static unsigned int unhex(char *pgm, char c)
{
	if ((c >= '0') && (c <= '9'))
		return c - '0';
	if ((c >= 'A') && (c <= 'F'))
		return c - 'A' + 10;
	if ((c >= 'a') && (c <= 'f'))
		return c - 'a' + 10;
	die(pgm, "Argument not hexadecimal");
	return 0;
}

static void __add(unsigned int *addr, int a)
{
	int i;

	addr[2] += a;
	for(i=2; i>=0; i--) {
		if (addr[i] > 255) {
			addr[i-1] += addr[i] / 256;
			addr[i] %= 256;
		}
	}
}

int main(int argc, char **argv)
{
	int i = 1, j;
	unsigned int card;
	unsigned int ccw_addr[3];
	unsigned int addr[3];
	unsigned int size;
	unsigned int data_cards;

	if (argc != 6)
		die(argv[0], "Invalid number of args");

	if ((strlen(argv[1]) != 8) ||
	    (strlen(argv[2]) != 8))
		die(argv[0], "Invalid PSW argument length");

	size = get_file_size(argv[3]);
	if (!size)
		die(argv[0], "cannot get file size");

	if ((strlen(argv[4]) != 6) ||
	    (strlen(argv[5]) != 6))
		die(argv[0], "Invalid addr argument length");

	addr[0] = (unhex(argv[0], argv[4][0]) << 4) |
		   unhex(argv[0], argv[4][1]);
	addr[1] = (unhex(argv[0], argv[4][2]) << 4) |
		   unhex(argv[0], argv[4][3]);
	addr[2] = (unhex(argv[0], argv[4][4]) << 4) |
		   unhex(argv[0], argv[4][5]);

	ccw_addr[0] = (unhex(argv[0], argv[5][0]) << 4) |
		       unhex(argv[0], argv[5][1]);
	ccw_addr[1] = (unhex(argv[0], argv[5][2]) << 4) |
		       unhex(argv[0], argv[5][3]);
	ccw_addr[2] = (unhex(argv[0], argv[5][4]) << 4) |
		       unhex(argv[0], argv[5][5]);

	/* round up */
	if (size % 80)
		size = size - (size % 80) + 80;

	data_cards = size / 80;

	printf(deck_head);

	printf(card_head, i);
	printf(psw_head);
	printf(psw,
	       argv[1][0], argv[1][1], argv[1][2], argv[1][3],
	       argv[1][4], argv[1][5], argv[1][6], argv[1][7],
	       argv[2][0], argv[2][1], argv[2][2], argv[2][3],
	       argv[2][4], argv[2][5], argv[2][6], argv[2][7]);

	if (data_cards == 1)
		die(argv[0], "Cannot handle the case where size <= 80");

	card = 2;
	i = 0;
	j = 1;

	printf(psw_tail, ADDR(ccw_addr), ADDRS(ccw_addr), CHAIN,
	       ADDR(ccw_addr), ADDRS(ccw_addr));
	printf(pad);
	__add(ccw_addr, 80);

	for(i=0; i<=((data_cards+9)/10) + (((data_cards+9)/10)+9)/10 - 2; i++) {
		printf(cp_ccw, i+3, ADDRS(ccw_addr));
		__add(ccw_addr, 80);
	}

	while (i % 10) {
		printf(nop_ccw, i);
		i++;
	}

	for(i=1; i<=data_cards; i++) {
		printf(data_ccw, i, ADDRS(addr),
		       (i == data_cards) ? 0 : CHAIN);

		__add(addr, 80);
	}

	return 0;

	printf(card_head, card++);
	return 0;
}
