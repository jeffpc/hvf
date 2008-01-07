#
# HVF: Hobbyist Virtualization Facility
#

VERSION=0.10

AS=as
CC=gcc
LD=ld
OBJCOPY=objcopy

MAKEFLAGS += -rR --no-print-directory
CFLAGS=-DVERSION=\"$(VERSION)\" -g -fno-strict-aliasing -fno-builtin -nostdinc -nostdlib -Wall -m64 -I include/ -O2
NUCLEUSCFLAGS=-include include/nucleus.h
LDFLAGS=-m elf64_s390

export AS CC LD OBJCOPY
export MAKEFLAGS CFLAGS NUCLEUSCFLAGS LDFLAGS

TOP_DIRS=nucleus/ mm/ lib/ drivers/

.PHONY: all build clean mrproper cleanup hvfclean iplclean tags
.PHONY: ipl/ $(TOP_DIRS)

all: build 
	@$(MAKE) hvf
	@$(MAKE) ipl/

hvf: $(patsubst %/,%/built-in.o,$(TOP_DIRS))
	$(LD) $(LDFLAGS) -T scripts/linker.script -o $@ $^

clean:
	@$(MAKE) DIR=nucleus/ cleanup
	@$(MAKE) DIR=mm/ cleanup
	@$(MAKE) DIR=lib/ cleanup
	@$(MAKE) DIR=drivers/ cleanup
	rm -f hvf
	rm -f loader.bin ipl/*.o ipl/*.rto ipl/.*.o ipl/ipl.S

mrproper: clean
	rm -f cscope.out ctags

cleanup:
	rm -f $(DIR)*.o

build: $(TOP_DIRS)

$(TOP_DIRS): %/:
	@$(MAKE) -f scripts/Makefile.build DIR=$@

tags:
	cscope -R -b

#
# Include Makefiles from all the top level directories
#
include $(patsubst %/,%/Makefile,$(TOP_DIRS))

#
# IPL specific bits
#

.PRECIOUS: ipl/%.o

ipl/: loader.bin
	@echo -n

loader.bin: ipl/ipl.rto ipl/setmode.rto ipl/loader.rto
	cat ipl/ipl.rto > "$@"
	cat ipl/setmode.rto >> "$@"
	cat ipl/loader.rto >> "$@"

ipl/%.rto: ipl/%.o
	$(OBJCOPY) -O binary -j .text $< $@

ipl/%.o: ipl/%.S
	$(AS) -m31 -o $@ $<

ipl/ipl.S: ipl/ipl.S_in ipl/setmode.rto ipl/loader.rto scripts/gen_ipl_s.sh
	bash scripts/gen_ipl_s.sh "$<" "$@"

ipl/loader.rto: ipl/loader.o
	$(OBJCOPY) -O binary -j .text -j .data -j .rodata $< $@

ipl/loader.o: ipl/loader.c ipl/loader_asm.S hvf
	$(AS) -m64 -o ipl/.loader_asm.o ipl/loader_asm.S
	$(CC) $(CFLAGS) -DBLOCK_SIZE=4096 -DBYTES_TO_READ=`stat -c %s hvf` -c -o ipl/.loader.o ipl/loader.c
	$(LD) -melf64_s390 -o $@ -T ipl/linker.script ipl/.loader.o ipl/.loader_asm.o
