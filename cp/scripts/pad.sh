#!/bin/bash

# pad <fname> <multiple>

len=`stat -c %s "$1"`
dif=`expr $len % "$2"`

if [ $dif -ne 0 ]; then
	dif=`expr "$2" - $dif`
	dd if=/dev/zero bs=1 count=$dif 2> /dev/null >> "$1"
fi
