AS=$(CROSS_COMPILE)as
OBJCOPY=$(CROSS_COMPILE)objcopy
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
CFLAGS=-g -fno-strict-aliasing -fno-builtin -nostdlib -Wall -m64 -I ../include/ -O2 -include ../include/types.h
NUCLEUSCFLAGS=
LIBS=string ebcdic

include ../cp/scripts/Makefile.commands

all: rdr.rto loader.rto

clean:
	$(call clean,*.o)
	$(call clean,*.rto)
	$(call clean,rdr.S)

loader.o: setmode.o loader_c.o loader_asm.o cpio.o malloc.o edf.o
	$(call link,$^ $(patsubst %,../lib/%.a,$(LIBS)),$@,-melf64_s390 -T linker.script)

rdr.S: loader.rto
	../build/ccw_gen 00080000 80100000 `stat -c %s $<` 100000 080000 > $@

%.o: %.c
	$(call c-to-o,$<,$@)

%.rto: %.o
	$(call objcopy-tdr,$<,$@)

%.o: %.S
	$(call s-to-o,$<,$@)

.PRECIOUS: %.o
