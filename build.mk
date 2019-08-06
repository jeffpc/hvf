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

HVF_VERSION=	0.16-rc3

SRCTOP!=	hg root

OBJCOPY=	s390x-linux-objcopy
SECTIONS?=	.text .data .rodata .rodata.str1.2

CC=		s390x-linux-gcc
CPPFLAGS.ALL=	-DVERSION="\"$(HVF_VERSION)\"" \
		-I $(SRCTOP)/include \
		-include $(SRCTOP)/include/types.h \
		$(DEFS)
CPPFLAGS.31=	$(CPPFLAGS.ALL)
CPPFLAGS.64=	$(CPPFLAGS.ALL)
CFLAGS.OPT?=	-O2
CFLAGS.ALL=	-Wall \
		$(CFLAGS.OPT) \
		-g \
		-m$(BITS) \
		-mbackchain \
		-msoft-float \
		-fno-strict-aliasing \
		-fno-builtin \
		-nostartfiles \
		-nostdlib \
		-nostdinc
CFLAGS.31=	$(CFLAGS.ALL) $(CPPFLAGS.31)
CFLAGS.64=	$(CFLAGS.ALL) $(CPPFLAGS.64)

LD=		s390x-linux-ld
LINKER_SCRIPT=	linker.script
LDFLAGS.ALL=	-T$(LINKER_SCRIPT)
LDFLAGS.31=	$(LDFLAGS.ALL) -melf_s390
LDFLAGS.64=	$(LDFLAGS.ALL) -melf64_s390

AR=		s390x-linux-ar

C_SRCS:=${SRCS:M*.c}
S_SRCS:=${SRCS:M*.s}
Y_SRCS:=${SRCS:M*.y}
L_SRCS:=${SRCS:M*.l}

GENSRCS:=${Y_SRCS:C/\.y$/.c/} \
	 ${L_SRCS:C/\.l$/.c/}
GENHDRS:=${Y_SRCS:C/\.y$/.h/} \
	 ${L_SRCS:C/\.l$/.h/}

OBJS:=${C_SRCS:%.c=%.o} \
      ${S_SRCS:%.s=%.o} \
      ${GENSRCS:%.c=%.o}

all: bin lib

clean:
.if defined(BIN)
	rm -f $(BIN) $(BIN).raw
.endif
.if defined(LIB)
	rm -f $(LIB).a
.endif
	rm -f $(OBJS) $(CLEANSRCS)
.if !empty(GENSRCS) || !empty(GENHDRS)
	rm -f $(GENSRCS) $(GENHDRS)
.endif

.if defined(BIN)
bin: $(BIN)
$(BIN): $(OBJS) $(LINKER_SCRIPT)
	$(LD) $(LDFLAGS.$(BITS)) -o $(BIN) $(OBJS) $(LD_ADD)

$(BIN).raw: $(BIN)
	$(OBJCOPY) -O binary ${SECTIONS:%=-j %} ${.ALLSRC} ${.TARGET}
.else
bin:
.endif

.if defined(LIB)
lib: $(LIB).a
$(LIB).a: $(OBJS)
	$(AR) cr ${.TARGET} ${.ALLSRC}
.else
lib:
.endif()

# remove built-in .[yl] -> .o rules
.y.o:
.l.o:

.c.o:
	$(CC) $(CFLAGS.$(BITS)) -c -o ${.TARGET} ${.IMPSRC}

.s.o:
	$(CC) $(CFLAGS.$(BITS)) -D_ASM -x assembler-with-cpp -c -o ${.TARGET} ${.IMPSRC}

.y.c:
	cd ${.IMPSRC:H} && $(SRCTOP)/build/byacc/yacc \
		-b ${.IMPSRC:T:R} \
		-d -P -p ${.IMPSRC:T:R:S/grammar//} \
		-o ${.IMPSRC:T:R}.c ${.IMPSRC:T}

.l.c:
	cd ${.IMPSRC:H} && $(SRCTOP)/build/re2c/re2c \
		-o ${.IMPSRC:T:R}.c ${.IMPSRC:T}
