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

.PHONY: all clean

all: $(BINS)
	@echo "Image is `stat -c %s dbg3215` bytes"

clean:
	rm -f $(common_OBJS) $(dbg3215_OBJS)
	rm -f dbg3215

dbg3215: $(common_OBJS) $(dbg3215_OBJS) $(common_LIBS)
	$(LD) $(LDFLAGS) -T linker.script -o $@ $^

%.o: %.S
	$(AS) -m64 -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
