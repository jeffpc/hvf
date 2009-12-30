#
# HVF: Hobbyist Virtualization Facility
#

#
# This is the top-level makefile not much happens here
#

all:
	make -C lib
	make -C cp
	make -C nss/8ball

doc:
	make -C doc/manual

clean:
	make -C lib clean
	make -C cp clean
	make -C nss/8ball clean
	make -C doc/manual clean

.PHONY: all doc clean
