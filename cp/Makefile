#
# Copyright (c) 2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

BIN=		hvf
SRCS=		\
		drivers/3215.c \
		drivers/dasd.c \
		drivers/device.c \
		drivers/spooldev.c \
		drivers/spooldev_cons.c \
		drivers/vdevice.c \
		fs/bcache.c \
		fs/bdev.c \
		fs/edf.c \
		fs/spool.c \
		guest/attach.c \
		guest/exception.c \
		guest/init.c \
		guest/instruction.c \
		guest/instruction_priv.c \
		guest/intercept.c \
		guest/ipl.c \
		guest/reset.c \
		guest/run.c \
		guest/system.c \
		mm/buddy.c \
		mm/dat.c \
		mm/page.c \
		mm/slab.c \
		nucleus/config.c \
		nucleus/config_grammar.y \
		nucleus/config_lexer.l \
		nucleus/direct.c \
		nucleus/direct_grammar.y \
		nucleus/direct_lexer.l \
		nucleus/ext.c \
		nucleus/init.c \
		nucleus/int.s \
		nucleus/io.c \
		nucleus/ldep.c \
		nucleus/mch.c \
		nucleus/mutex.c \
		nucleus/pgm.c \
		nucleus/printf.c \
		nucleus/sched.c \
		nucleus/sclp.c \
		nucleus/spinlock.c \
		nucleus/svc.c \
		nucleus/symtab.c \
		nucleus/util.c \
		shell/cmds.c \
		shell/disassm.c \
		shell/init.c
BITS=		64

DEFS=		-I include \
		-include include/nucleus.h
LD_ADD=		$(SRCTOP)/lib/lib.a

.include <${.CURDIR:H}/build.mk>

nucleus/direct.c: nucleus/direct_grammar.c
