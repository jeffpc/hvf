#
# HVF: Hobbyist Virtualization Facility
#

VERSION=0.11

AS=as
CC=gcc
LD=ld
OBJCOPY=objcopy

# By default, be terse
V=0

MAKEFLAGS += -rR --no-print-directory
CFLAGS=-DVERSION=\"$(VERSION)\" -g -fno-strict-aliasing -fno-builtin -nostdinc -nostdlib -Wall -m64 -I include/ -O2
NUCLEUSCFLAGS=-include include/nucleus.h
LDFLAGS=-m elf64_s390

export AS CC LD OBJCOPY
export MAKEFLAGS CFLAGS NUCLEUSCFLAGS LDFLAGS

TOP_DIRS=nucleus/ mm/ lib/ drivers/ cp/

.PHONY: all build clean mrproper cleanup hvfclean iplclean tags
.PHONY: ipl/ $(TOP_DIRS)

include scripts/Makefile.commands

all: build hvf
	@echo "Image is `stat -c %s hvf` bytes"
	@$(MAKE) ipl/ V=$V

hvf: $(patsubst %/,%/built-in.o,$(TOP_DIRS))
	$(call link-hvf,$^,$@)

clean:
	@$(MAKE) DIR=nucleus/ cleanup V=$V
	@$(MAKE) DIR=mm/ cleanup V=$V
	@$(MAKE) DIR=lib/ cleanup V=$V
	@$(MAKE) DIR=drivers/ cleanup V=$V
	@$(MAKE) DIR=cp/ cleanup V=$V
	$(call clean,hvf)
	$(call clean,loader.bin ipl/*.o ipl/*.rto ipl/.*.o ipl/ipl.S)

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

ipl/: loader.bin
	@echo -n

loader.bin: ipl/ipl.rto ipl/setmode.rto ipl/loader.rto
	$(call concat,ipl/ipl.rto ipl/setmode.rto ipl/loader.rto,$@)
	@echo "Loader is `stat -c %s loader.bin` bytes"

ipl/%.rto: ipl/%.o
	$(call objcopy-t,$<,$@)

ipl/%.o: ipl/%.S
	$(call s-to-o-31,$<,$@)

ipl/ipl.S: ipl/ipl.S_in ipl/setmode.rto ipl/loader.rto scripts/gen_ipl_s.sh
	$(call genipl-s,$<,$@)

ipl/loader.rto: ipl/loader.o
	$(call objcopy-tdr,$<,$@)

ipl/loader.o: ipl/loader.c ipl/loader_asm.S hvf
	$(call s-to-o,ipl/loader_asm.S,ipl/.loader_asm.o)
	$(call c-to-o-ipl,ipl/loader.c,ipl/.loader.o,4096,`stat -c %s hvf`)
	$(call link-ipl,ipl/.loader.o ipl/.loader_asm.o,$@)
