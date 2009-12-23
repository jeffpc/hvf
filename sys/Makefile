#
# HVF: Hobbyist Virtualization Facility
#

VERSION=0.16-rc2

AS=$(CROSS_COMPILE)as
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
OBJCOPY=$(CROSS_COMPILE)objcopy

# By default, be terse
V=0

DISPLAYVERSION=$(shell ./scripts/extract-version.sh)

MAKEFLAGS += -rR --no-print-directory
CFLAGS=-DVERSION=\"$(DISPLAYVERSION)\" -g -fno-strict-aliasing -fno-builtin -nostdlib -Wall -m64 -I include/ -I ../include/ -O2 -include ../include/types.h
NUCLEUSCFLAGS=-include include/nucleus.h
LDFLAGS=-m elf64_s390

LIBS=clock digest string

export AS CC LD OBJCOPY
export MAKEFLAGS CFLAGS NUCLEUSCFLAGS LDFLAGS

TOP_DIRS=nucleus/ mm/ fs/ drivers/ cp/

.PHONY: all build clean mrproper cleanup hvfclean iplclean tags
.PHONY: ipl/ $(TOP_DIRS)

include scripts/Makefile.commands

all: build hvf
	@echo "Image is `stat -c %s hvf` bytes"
	@$(MAKE) ipl/ V=$V

hvf: $(patsubst %/,%/built-in.o,$(TOP_DIRS))
	$(call link-hvf,$^ $(patsubst %,../lib/%.a,$(LIBS)),$@)

docs:
	./scripts/gen-docs.sh

clean:
	@$(MAKE) DIR=nucleus/ cleanup V=$V
	@$(MAKE) DIR=mm/ cleanup V=$V
	@$(MAKE) DIR=fs/ cleanup V=$V
	@$(MAKE) DIR=drivers/ cleanup V=$V
	@$(MAKE) DIR=cp/ cleanup V=$V
	$(call clean,hvf)
	$(call clean,loader_*.bin ipl/*.o ipl/*.rto ipl/.*.o ipl/ipl_tape.S ipl/ipl_rdr_ccws.S)
	$(call clean,cp/directory_structs.c)
	$(call rclean,doc/commands)

mrproper: clean
	$(call clean,cscope.out ctags)

cleanup:
	$(call clean,$(DIR)*.o)

build: $(TOP_DIRS)

$(TOP_DIRS): %/:
	@$(MAKE) -f scripts/Makefile.build DIR=$@ V=$V

tags:
	$(call cscope)

#
# Include Makefiles from all the top level directories
#
include $(patsubst %/,%/Makefile,$(TOP_DIRS))

#
# IPL specific bits
#

.PRECIOUS: ipl/%.o

ipl/: loader_rdr.bin loader_tape.bin
	@echo -n

loader_rdr.bin: ipl/ipl_rdr.rto ipl/ipl_rdr_ccws.rto ipl/setmode.rto ipl/loader_rdr.rto
	$(call concat,$^,$@)
	$(call pad,$@,80)
	@echo "Card reader loader is `stat -c %s $@` bytes"

loader_tape.bin: ipl/ipl_tape.rto ipl/setmode.rto ipl/loader_tape.rto
	$(call concat,$^,$@)
	@echo "Tape loader is `stat -c %s $@` bytes"

ipl/loader_asm.o: ipl/loader_asm.S
	$(call s-to-o,$<,$@)

ipl/ipl_tape.S: ipl/ipl_tape.S_in ipl/setmode.rto ipl/loader_tape.rto scripts/gen_tape_ipl_s.sh
	$(call genipl-tape,ipl/ipl_tape.S_in,$@)

ipl/ipl_rdr_ccws.S: ipl/setmode.rto ipl/loader_rdr.rto
	$(call genipl-rdr,$@)

ipl/loader_%.rto: ipl/loader_%.o
	$(call objcopy-tdr,$<,$@)

ipl/loader_tape.o: ipl/loader.c ipl/loader_asm.o hvf
	$(call c-to-o-ipl,ipl/loader.c,ipl/loader_c_tape.o,4096,`stat -c %s hvf`,-DTAPE_SEEK)
	$(call link-ipl,ipl/loader_c_tape.o ipl/loader_asm.o,$@)

ipl/loader_rdr.o: ipl/loader.c ipl/loader_asm.o hvf
	$(call c-to-o-ipl,ipl/loader.c,ipl/loader_c_rdr.o,80,`stat -c %s hvf`,)
	$(call link-ipl,ipl/loader_c_rdr.o ipl/loader_asm.o,$@)

ipl/%.rto: ipl/%.o
	$(call objcopy-t,$<,$@)

ipl/%.o: ipl/%.S
	$(call s-to-o,$<,$@)

