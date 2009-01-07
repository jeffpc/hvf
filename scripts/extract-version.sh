#!/bin/sh

v=`git describe 2> /dev/null`

case "$v" in
	"")
		awk '/^VERSION=/ { print substr($0,9); exit; }' < Makefile
		;;
	*)
		echo "$v"
		;;
esac
