/*
 * Copyright (c) 2007-2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <arch.h>
#include <string.h>
#include "int.h"
#include "misc.h"

/*
 * This guest OS has a very simple purpose.  It is responsible for
 * displaying the login splash, prompting the user for a username and
 * password, and then querying CP to verify that the password is correct.
 * If correct, CP will detach the rdev from this guest, and attach it to the
 * user's guest.
 *
 *         +------------------------------------------------+
 *         | +------------------------------------+         |
 *         | | virt_sys                           |         |
 *         | | +--------+ +-------+ +---------+   |   +----+|
 *         | | |virt_cpu| |storage| |virt_cons|<--+-->|rdev||
 *         | | +--------+ +-------+ +---------+   |   +----+|
 *         | |                           /\       |         |
 *         | |                           ||       |         |
 *         | |                           \/       |         |
 *         | | +--------+ +--------+ +--------+   |         |
 *         | | |virt_dev| |virt_dev| |virt_dev|   |         |
 *         | | +--------+ +--------+ +--------+   |         |
 *         | +------------------------------------+         |
 *         |                                                |
 *         | +------------------------------------+         |
 *         | | virt_sys                           |         |
 *         | | +--------+ +-------+ +---------+   |   +----+|
 *         | | |virt_cpu| |storage| |virt_cons|<--+-->|rdev||
 *         | | +--------+ +-------+ +---------+   |   +----+|
 *         | |                           /\       |         |
 *         | |                           ||       |         |
 *         | |                           \/       |         |
 *         | | +--------+ +--------+ +--------+   |         |
 *         | | |virt_dev| |virt_dev| |virt_dev|   |         |
 *         | | +--------+ +--------+ +--------+   |         |
 *         | +------------------------------------+         |
 *         |                                                |
 *         |                   :                            |
 *         |                   :                            |
 *         |                   :                            |
 *         |                                                |
 *         | +------------------------------------+         |
 *         | | virt_sys (*LOGIN)                  |         |
 *         | | +--------+ +-------+ +---------+   |         |
 *         | | |virt_cpu| |storage| |virt_cons|   |         |
 *         | | +--------+ +-------+ +---------+   |         |
 *         | |                       +--------+   |   +----+|
 *         | |                       |virt_dev|<--+-->|rdev||
 *         | |                       +--------+   |   +----+|
 *         | |                       +--------+   |   +----+|
 *         | |                       |virt_dev|<--+-->|rdev||
 *         | |                       +--------+   |   +----+|
 *         | |                       +--------+   |   +----+|
 *         | |                       |virt_dev|<--+-->|rdev||
 *         | |                       +--------+   |   +----+|
 *         | +------------------------------------+         |
 *         +------------------------------------------------+
 *
 *
 * In other words, here are the actions that CP takes:
 *
 * host IPL:
 *  - ipl *LOGIN
 *  - attach operator console to *LOGIN
 *
 * login:
 *  - alloc virt sys, virt cpu, virt devices, virt cons, storage
 *  - detach console rdev from *LOGIN
 *  - attach console rdev to virt cons
 *
 * logout/force:
 *  - free virt sys, virt cpu, virt devices, virt cons, storage
 *  - attach console rdev to *LOGIN
 *
 * disconnect:
 *  - detach console rdev from virt cons
 *  - attach console rdev to *LOGIN
 *
 * reconnect:
 *  - detach console rdev from *LOGIN
 *  - attach console rdev to virt cons
 */

struct psw disabled_wait = { .w = 1, .ea = 1, .ba = 1, .ptr = 0xfafafa, };
struct psw enabled_wait = { .io = 1, .m = 1, .w = 1, .ea = 1, .ba = 1, };

static struct psw __new_rst = { .ea = 1, .ba = 1, .ptr = ADDR64(&rst_int), };
static struct psw __new_ext = { .ea = 1, .ba = 1, .ptr = ADDR64(&ext_int), };
static struct psw __new_svc = { .ea = 1, .ba = 1, .ptr = ADDR64(&svc_int), };
static struct psw __new_prg = { .ea = 1, .ba = 1, .ptr = ADDR64(&prg_int), };
static struct psw __new_mch = { .ea = 1, .ba = 1, .ptr = ADDR64(&mch_int), };
static struct psw __new_io  = { .ea = 1, .ba = 1, .ptr = ADDR64(&io_int), };

static void init_io(void)
{
	u64 cr6;

	cr6 = get_cr(6);

	/* enable all I/O interrupt classes */
	cr6 |= BIT64(32);
	cr6 |= BIT64(33);
	cr6 |= BIT64(34);
	cr6 |= BIT64(35);
	cr6 |= BIT64(36);
	cr6 |= BIT64(37);
	cr6 |= BIT64(38);
	cr6 |= BIT64(39);

	set_cr(6, cr6);
}

static void init_mch(void)
{
	u64 cr14;

	cr14 = get_cr(14);

	/* enable Channel-Report-Pending Machine Check Interruption subclass */
	cr14 |= BIT64(35);

	set_cr(14, cr14);
}

void start()
{
	/* set up interrupt PSWs */
	memcpy(&psw_rst_new, &__new_rst, sizeof(struct psw));
	memcpy(&psw_ext_new, &__new_ext, sizeof(struct psw));
	memcpy(&psw_svc_new, &__new_svc, sizeof(struct psw));
	memcpy(&psw_prg_new, &__new_prg, sizeof(struct psw));
	memcpy(&psw_mch_new, &__new_mch, sizeof(struct psw));
	memcpy(&psw_io_new,  &__new_io,  sizeof(struct psw));

	/* enable all 8 I/O classes */
	init_io();

	init_mch();

	/* this enables I/O & MCH interrupts */
	lpswe(&enabled_wait);

	die();
}
