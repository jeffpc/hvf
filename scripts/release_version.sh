#!/bin/bash

if [ $# -ne 1 ]; then
	echo "Usage: $0 <tagname>"
	exit 1
fi

case "$1" in
	v*)
		tag="$1"
		;;
	[0-9]*)
		tag="v$1"
		;;
esac

ver=`echo $tag | sed -e 's/^v//'`

echo "About start release process of '`git rev-parse HEAD`' as '$tag'..."
echo "Press enter to continue."
read n

echo "1) Edit VERSION in 'Makefile'"
read n
vim Makefile
git update-index Makefile
git commit -s -m "HVF $ver"

echo "2) Tag the commit with '$tag'"
read n
git tag -u C7958FFE -m "HVF $tag" "$tag"

echo "3) Generate hvf-$ver.tar.{gz,bz2}"
read n
git archive --format=tar --prefix=hvf-$ver/ HEAD | gzip -9 > hvf-$ver.tar.gz
git archive --format=tar --prefix=hvf-$ver/ HEAD | bzip2 -9 > hvf-$ver.tar.bz2

echo "4) Profit"

echo "We're all done, have a nice day."
