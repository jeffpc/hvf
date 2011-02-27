#
# HVF: Hobbyist Virtualization Facility
#

#
# This is the top-level makefile not much happens here
#

all:
	make -C build
	make -C lib
	make -C cp
	make -C nss/8ball
	make -C loader
	make -C installer

doc:
	make -C doc/manual

clean:
	make -C build clean
	make -C lib clean
	make -C cp clean
	make -C nss/8ball clean
	make -C loader clean
	make -C installer clean
	make -C doc/manual clean

.PHONY: all doc clean
