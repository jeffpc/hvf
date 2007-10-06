#!/bin/bash
#
# Copyright (c) 2007 Josef 'Jeff' Sipek
#

len=80 # this is going the be the size of ipl.rto
len=$(expr $len + `stat -c %s ipl/setmode.rto`)
len=$(expr $len + `stat -c %s ipl/loader.rto`)
dif=`expr $len % 80`

if [ $dif -ne 0 ]; then
	len=`expr $len - $dif`
	len=`expr $len + 80`
fi

if [ $len -gt `expr 65536` ]; then
	echo "ERROR: loader code is too long" >&2
	exit 1
fi

ccw2=$len

ccw2h=`expr $ccw2 / 256`
ccw2l=`expr $ccw2 % 256`

sed -e "s/CCW2H/$ccw2h/g" \
    -e "s/CCW2L/$ccw2l/g" \
    "$1" > "$2"
