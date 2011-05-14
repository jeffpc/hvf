LD=$(CROSS_COMPILE)ld
AS=$(CROSS_COMPILE)as
OBJCOPY=$(CROSS_COMPILE)objcopy

include ../../cp/scripts/Makefile.commands

.PHONY: all clean

all: 8ball
	$(call objcopy-tdr,8ball,8ball.raw)

8ball: 8ball.o
	$(call link,$^,$@,-melf_s390 -T linker-8ball.script)

8ball.o: 8ball.S
	$(call s-to-o,$<,$@,-m31)

clean:
	$(call clean,8ball 8ball.o 8ball.raw)
