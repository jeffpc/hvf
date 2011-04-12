HOSTCC=gcc
CFLAGS=-Wall -O2 -g

all: ccw_gen padcat

clean:
	rm -f ccw_gen padcat

ccw_gen: ccw_gen.c
	$(HOSTCC) $(CFLAGS) -o $@ $<

padcat: padcat.c
	$(HOSTCC) $(CFLAGS) -o $@ $<
