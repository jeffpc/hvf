AS=$(CROSS_COMPILE)as
OBJCOPY=$(CROSS_COMPILE)objcopy
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
CFLAGS=-g -fno-strict-aliasing -fno-builtin -nostdlib -Wall -m64 -I ../include/ -O2 -include ../include/types.h
NUCLEUSCFLAGS=

include ../cp/scripts/Makefile.commands

all: eckd.rto loader.rto

clean:
	$(call clean,*.o)
	$(call clean,*.rto)

loader.o: setmode.o loader_c.o loader_asm.o
	$(call link,$^,$@,-melf64_s390 -T linker.script)

%.o: %.c
	$(call c-to-o,$<,$@)

%.rto: %.o
	$(call objcopy-tdr,$<,$@)

%.o: %.S
	$(call s-to-o,$<,$@)

.PRECIOUS: %.o
