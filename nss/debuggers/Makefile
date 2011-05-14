AS=$(CROSS_COMPILE)as
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
AR=$(CROSS_COMPILE)ar
OBJCOPY=$(CROSS_COMPILE)objcopy

CFLAGS=-Wall -O2 -g -fno-strict-aliasing -fno-builtin -nostdlib -m64 \
       -include ../../include/types.h -I include/ -I ../../include/
LDFLAGS=-m elf64_s390

BINS=dbg3215

common_OBJS=src/init.o src/io.o src/ioint.o
common_LIBS=../../lib/ebcdic.a ../../lib/string.a
dbg3215_OBJS=src/3215.o src/cons.o

include ../../cp/scripts/Makefile.commands

.PHONY: all clean

all: $(BINS)

clean:
	$(call clean,$(common_OBJS) $(dbg3215_OBJS))
	$(call clean,dbg3215)

dbg3215: $(common_OBJS) $(dbg3215_OBJS) $(common_LIBS)
	$(call link,$^,$@,-melf64_s390 -T linker.script)

%.o: %.S
	$(call s-to-o,$<,$@)

%.o: %.c
	$(call c-to-o,$<,$@)
