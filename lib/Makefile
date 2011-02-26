AS=$(CROSS_COMPILE)as
CC=$(CROSS_COMPILE)gcc
CFLAGS=-g -fno-strict-aliasing -fno-builtin -nostdlib -Wall -m64 -I ../include/ -include ../include/types.h -O2
NUCLEUSCFLAGS=

LIBS=clock.a		\
     digest.a		\
     string.a		\
     ebcdic.a

include ../cp/scripts/Makefile.commands

all: $(LIBS)

clock.a: clock.o
	$(call ar,$@,$^)

digest.a: digest.o
	$(call ar,$@,$^)

string.a: string.o vsprintf.o
	$(call ar,$@,$^)

ebcdic.a: ebcdic.o
	$(call ar,$@,$^)

%.o: %.c
	$(call c-to-o,$<,$@)

clean:
	$(call clean,*.o *.a)
