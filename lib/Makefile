CC=s390x-linux-gcc
AR=s390x-linux-ar
CFLAGS=-g -fno-strict-aliasing -fno-builtin -nostdinc -nostdlib -Wall -m64 -I ../include/ -include ../include/types.h -O2

LIBS=clock.a		\
     digest.a

all: $(LIBS)

clock.a: clock.o
	$(AR) rc $@ $<

digest.a: digest.o
	$(AR) rc $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.a
