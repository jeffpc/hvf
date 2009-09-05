LD=s390x-linux-ld
OBJCOPY=s390x-linux-objcopy
AS=s390x-linux-as

all:
	$(AS) -o 8ball.o 8ball.S
	$(LD) $(LDFLAGS) -T linker-8ball.script -o 8ball 8ball.o
	$(OBJCOPY) -O binary -j .text -j .data -j .rodata 8ball 8ball.raw

clean:
	rm -f 8ball 8ball.o 8ball.raw
