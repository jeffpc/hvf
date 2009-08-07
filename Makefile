#
# HVF: Hobbyist Virtualization Facility
#

#
# This is the top-level makefile not much happens here
#

all:
	make -C sys
	make -C doc/manual

clean:
	make -C sys clean
	make -C doc/manual clean
