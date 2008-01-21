#!/bin/bash
#
# Copyright (c) 2008 Josef 'Jeff' Sipek
#

len=`stat -c %s ipl/setmode.rto`
len=`expr $len + $(stat -c %s ipl/loader_rdr.rto)`
dif=`expr $len % 80`

if [ $dif -ne 0 ]; then
	len=`expr $len - $dif`
	len=`expr $len + 80`
fi

len=`expr $len \/ 80`

# 13 CCWs * 80 bytes/ccw = 1040 bytes
if [ $len -gt 13 ]; then
	echo "ERROR: loader code is too long" >&2
	exit 1
fi

echo -n "" > "$1"

addr=8388608 # == 0x800000
flags="0x60"
for x in `seq 1 $len`; do
	high_addr=`expr $addr \/ 65536`
	mid_addr=`expr $(expr $addr \/ 256) % 256`
	low_addr=`expr $addr % 256`

	[ $x -eq $len ] && flags="0x20"

	echo "# CCW $x" >> "$1"
	echo "	.byte	0x02, $high_addr, $mid_addr, $low_addr" >> "$1"
	echo "	.byte	$flags, 0x00, 0x00, 0x50" >> "$1"
	echo "" >> "$1"

	addr=`expr $addr + 80`
done

echo "# pad" >> "$1"
for x in `seq $len 19`; do
	echo "	.byte	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40" >> "$1"
done
