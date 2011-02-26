#!/bin/sh

LAST_CARD_NO=37

s=`stat -c %s loader.rto`
max=`expr $LAST_CARD_NO \* 80`
min=`expr $max - 80`

if [ $s -lt $min ]; then
	dif=`expr $min - $s`
	echo "loader.rto is $s, which is $dif bytes less than min supported $min"
	exit 1
elif [ $s -gt $max ]; then
	dif=`expr $s - $max`
	echo "loader.rto is $s, which is $dif bytes more than max supported $max"
	exit 1
fi
