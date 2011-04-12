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
	./build/mkarchive \
		cp/hvf.directory text 80 \
		cp/hvf bin \
		loader/eckd.rto bin \
		loader/loader.rto bin \
		> installer/archive.cpio
	./build/padcat installer/rdr.rto installer/loader.rto installer/archive.cpio > installer.bin

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
	rm -f installer/archive.cpio
	rm -f installer.bin

.PHONY: all doc clean
