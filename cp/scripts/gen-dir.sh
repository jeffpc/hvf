#!/bin/sh

if [ $# -ne 2 ]; then
	echo "Usage: $0 <directory> <outfile>" >&2
	exit 1
fi

userid=""

close_devlist()
{
cat >> directory.devlist <<DONE
	{ /* END   */ .type = VDEV_INVAL, },
};
DONE
echo "	}," >> directory.userlist
}

parse_line()
{
	case "$1" in
		USER)
			[ ! -z "$userid" ] && close_devlist

			echo "static struct directory_vdev __directory_$2[] = {" >> directory.devlist
			echo "	{" >> directory.userlist
			echo "		.userid = \"$2\"," >> directory.userlist
			echo "		.auth = '$3'," >> directory.userlist
			echo "		.devices = __directory_$2," >> directory.userlist

			userid="$2"
			;;
		MACHINE)
			;;
		STORAGE)
			tmp=`echo $2 | sed -e 's/[KMG]//g'`
			case "$2" in
				*K) size="${tmp}ULL * 1024ULL" ;;
				*M) size="${tmp}ULL * 1024ULL * 1024ULL" ;;
				*G) size="${tmp}ULL * 1024ULL * 1024ULL * 1024ULL" ;;
				*)  size="$2ULL" ;;
			esac
			echo "		.storage_size = $size," >> directory.userlist
			;;
		CONSOLE)
			echo "	{ /* CON   */ .type = VDEV_CONS,  .vdev = 0x$2, }," >> directory.devlist
			;;
		SPOOL)
			case "$4" in
				READER)
					echo "	{ /* RDR   */ .type = VDEV_SPOOL, .vdev = 0x$2, .u.spool = { .type = 0x$3, .model = 1 }, }," >> directory.devlist
					;;
				PUNCH)
					echo "	{ /* PUN   */ .type = VDEV_SPOOL, .vdev = 0x$2, .u.spool = { .type = 0x$3, .model = 1 }, }," >> directory.devlist
					;;
				PRINT)
					echo "	{ /* PRT   */ .type = VDEV_SPOOL, .vdev = 0x$2, .u.spool = { .type = 0x$3, .model = 1 }, }," >> directory.devlist
					;;
			esac
			;;
		MDISK)
			echo "	{ /* MDISK */ .type = VDEV_MDISK, .vdev = 0x$2, .u.mdisk = { .rdev = 0x$6, .cyloff = $4, .cylcnt = $5, }, }," >> directory.devlist
			;;
		DEDICATE)
			echo "	{ /* DED   */ .type = VDEV_DED,   .vdev = 0x$2, .u.dedicate = { .rdev = 0x$3, }, }," >> directory.devlist
			;;
		*)
			echo "Error near:" "$@" >&2
			exit 2
			;;
	esac
}

open_userlist()
{
	echo "static struct user directory[] = {" > directory.userlist
}

close_userlist()
{
cat >> directory.userlist <<DONE
	{ /* END */ .userid = NULL, },
};
DONE
}

rm -f directory.devlist
open_userlist
cat "$1" | while read line ; do
	[ -z "$line" ] && continue

	parse_line $line
done
close_devlist
close_userlist

cat directory.devlist directory.userlist > $2

rm -f directory.devlist directory.userlist
