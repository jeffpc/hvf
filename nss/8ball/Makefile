LD=$(CROSS_COMPILE)ld
AS=$(CROSS_COMPILE)as
OBJCOPY=$(CROSS_COMPILE)objcopy

all:
	$(AS) -m31 -o 8ball.o 8ball.S
	$(LD) $(LDFLAGS) -melf_s390 -T linker-8ball.script -o 8ball 8ball.o
	$(OBJCOPY) -O binary -j .text -j .data -j .rodata 8ball 8ball.raw

clean:
	rm -f 8ball 8ball.o 8ball.raw
