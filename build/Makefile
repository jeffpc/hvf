HOSTCC=gcc
CFLAGS=-Wall -O2 -g

include ../cp/scripts/Makefile.commands

.PHONY: all clean

all: ccw_gen padcat mkarchive

clean:
	$(call clean,ccw_gen padcat mkarchive)

ccw_gen: ccw_gen.c
	$(HOSTCC) $(CFLAGS) -o $@ $<

padcat: padcat.c
	$(HOSTCC) $(CFLAGS) -o $@ $<

mkarchive: mkarchive.c
	$(HOSTCC) $(CFLAGS) -o $@ $<
