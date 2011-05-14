#ifndef __DIE_H
#define __DIE_H

#define die()	do { \
			asm volatile( \
				"SR	%r1, %r1	# not used, but should be zero\n" \
				"SR	%r3, %r3 	# CPU Address\n" \
				"SIGP	%r1, %r3, 0x05	# Signal, order 0x05\n" \
			); \
			for(;;); \
		} while(0)

#endif
