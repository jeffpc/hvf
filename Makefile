#
# Copyright (c) 2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

SUBDIRS=lib cp nss loader

all: build-installer

clean:
.for s in $(SUBDIRS)
	$(MAKE) -C $s clean
.endfor

build-tools:
	cd build && cmake . && $(MAKE)

clean-tools:
	cd build && make clean || true

build-os: build-tools
.for s in $(SUBDIRS)
	$(MAKE) -C $s
.endfor

clean-os:
.for s in $(SUBDIRS)
	$(MAKE) -C $s clean
.endfor

build-installer: build-os
	$(MAKE) -C installer
	${.CURDIR}/build/mkarchive \
		${.CURDIR}/cp/config/hvf.directory text 80 \
		${.CURDIR}/cp/config/system.config text 80 \
		${.CURDIR}/cp/config/local-3215.txt text 80 \
		${.CURDIR}/cp/hvf bin \
		${.CURDIR}/loader/eckd-ipl-rec/eckd.raw bin \
		${.CURDIR}/loader/loader/loader.raw bin \
		${.CURDIR}/doc/installed_files.txt text 80 \
		${.CURDIR}/nss/8ball/8ball bin \
		${.CURDIR}/nss/ipldev/ipldev bin \
		${.CURDIR}/nss/login/login bin \
		> ${.CURDIR}/installer.cpio
	${.CURDIR}/build/padcat \
		${.CURDIR}/installer/rdr-ipl-rec/rdr.raw \
		${.CURDIR}/installer/installer/installer.raw \
		${.CURDIR}/installer.cpio \
		> ${.CURDIR}/installer.bin

clean-installer:
	$(MAKE) -C installer clean
	rm -f installer.cpio installer.bin
