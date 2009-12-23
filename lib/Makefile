CC=s390x-linux-gcc
AR=s390x-linux-ar
CFLAGS=-g -fno-strict-aliasing -fno-builtin -nostdlib -Wall -m64 -I ../include/ -include ../include/types.h -O2
NUCLEUSCFLAGS=

LIBS=clock.a		\
     digest.a		\
     string.a

include ../sys/scripts/Makefile.commands

all: $(LIBS)

clock.a: clock.o
	$(call ar,$@,$^)

digest.a: digest.o
	$(call ar,$@,$^)

string.a: string.o vsprintf.o
	$(call ar,$@,$^)

%.o: %.c
	$(call c-to-o,$<,$@)

clean:
	$(call clean,*.o *.a)
