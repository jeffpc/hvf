HOSTCC=gcc
CFLAGS=-Wall -O2 -g

all: ccw_gen

clean:
	rm -f ccw_gen

ccw_gen: ccw_gen.c
	$(HOSTCC) $(CFLAGS) -o $@ $<
