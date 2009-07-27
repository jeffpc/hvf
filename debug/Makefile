CC=s390x-linux-gcc
LD=s390x-linux-ld
OBJCOPY=s390x-linux-objcopy
AS=s390x-linux-as
NM=s390x-linux-nm
CFLAGS=-Wall -O2 -g -fno-strict-aliasing -nostdlib

all:
	$(CC) $(CFLAGS) -c -o debug.o debug.c
	$(LD) $(LDFLAGS) -T linker-debug.script -o debug debug.o
	$(OBJCOPY) -O binary -j .text -j .data -j .rodata -j .rodata.str1.2 debug debug.raw
	$(AS) -m64 --defsym DEBUGADDR=0x$(shell $(NM) debug|grep start$$|cut -d ' ' -f1) -o loader.o loader.S
	$(LD) $(LDFLAGS) -T linker-loader.script -o loader loader.o
	$(OBJCOPY) -O binary -j .text -j .data -j .rodata loader loader.raw
