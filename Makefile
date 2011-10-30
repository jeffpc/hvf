#
# HVF: Hobbyist Virtualization Facility
#

#
# This is the top-level makefile not much happens here
#

all:
	$(MAKE) -C build
	$(MAKE) -C lib
	$(MAKE) -C cp
	$(MAKE) -C nss/8ball
	$(MAKE) -C loader
	$(MAKE) -C installer
	./build/mkarchive \
		cp/config/hvf.directory text 80 \
		cp/config/system.config text 80 \
		cp/config/local-3215.txt text 80 \
		cp/hvf bin \
		loader/eckd.rto bin \
		loader/loader.rto bin \
		doc/installed_files.txt text 80 \
		nss/8ball/8ball bin \
		> installer/archive.cpio
	./build/padcat installer/rdr.rto installer/loader.rto installer/archive.cpio > installer.bin

doc:
	$(MAKE) -C doc/manual

clean:
	$(MAKE) -C build clean
	$(MAKE) -C lib clean
	$(MAKE) -C cp clean
	$(MAKE) -C nss/8ball clean
	$(MAKE) -C loader clean
	$(MAKE) -C installer clean
	$(MAKE) -C doc/manual clean
	rm -f installer/archive.cpio
	rm -f installer.bin

.PHONY: all doc clean
