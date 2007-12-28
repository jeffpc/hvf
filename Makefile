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

.PRECIOUS: %.o

.PHONY: all build clean mrproper cleanup hvfclean iplclean tags

ifneq ($(DIR),)
.PHONY: $(DIR)
override DIR:=$(subst /,,$(DIR))
endif

TOP_DIRS=nucleus/ mm/ lib/ drivers/

all:
	@$(MAKE) DIR=nucleus/ build
	@$(MAKE) DIR=mm/ build
	@$(MAKE) DIR=lib/ build
	@$(MAKE) DIR=drivers/ build
	@$(MAKE) hvf
	@$(MAKE) ipl/

hvf: $(patsubst %/,%/built-in.o,$(TOP_DIRS))
	$(LD) $(LDFLAGS) -T linker.script -o $@ $^

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
	rm -f $(DIR)/*.o

build: $(DIR)/built-in.o

tags:
	cscope -R -b

#
# Include Makefiles from all the top level directories
#
include $(patsubst %/,%/Makefile,$(TOP_DIRS))

%/built-in.o: $(patsubst %,$(DIR)/%,$(objs-$(DIR)))
	$(LD) $(LDFLAGS) -r -o $@ $(patsubst %,$(DIR)/%,$(objs-$(DIR)))

%.o: %.S
	$(AS) -m64 -o $@ $<

%.s: %.c
	$(CC) $(CFLAGS) $(NUCLEUSCFLAGS) -S -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) $(NUCLEUSCFLAGS) -c -o $@ $<


#
# IPL specific bits
#

.PRECIOUS: ipl/%.o

ipl/: loader.bin

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
