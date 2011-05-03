#
# HVF: Hobbyist Virtualization Facility
#
# (C) Copyright 2007-2010  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
#
# This file is released under the GPLv2.  See the COPYING file for more
# details.
#

VERSION=0.16-rc3

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

LIBS=clock digest string ebcdic errno

export AS CC LD OBJCOPY
export MAKEFLAGS CFLAGS NUCLEUSCFLAGS LDFLAGS

TOP_DIRS=nucleus/ mm/ fs/ drivers/ shell/ guest/

.PHONY: all build clean mrproper cleanup tags
.PHONY: $(TOP_DIRS)

include scripts/Makefile.commands

all: build hvf
	@echo "Image is `stat -c %s hvf` bytes"

hvf: $(patsubst %/,%/built-in.o,$(TOP_DIRS))
	$(call link-hvf,$^ $(patsubst %,../lib/%.a,$(LIBS)),$@)

docs:
	./scripts/gen-docs.sh

clean:
	@$(MAKE) DIR=nucleus/ cleanup V=$V
	@$(MAKE) DIR=mm/ cleanup V=$V
	@$(MAKE) DIR=fs/ cleanup V=$V
	@$(MAKE) DIR=drivers/ cleanup V=$V
	@$(MAKE) DIR=shell/ cleanup V=$V
	@$(MAKE) DIR=guest/ cleanup V=$V
	$(call clean,hvf)
	$(call clean,shell/directory_structs.c)
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
