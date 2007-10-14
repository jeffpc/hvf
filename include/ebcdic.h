#ifndef __EBCDIC_H
#define __EBCDIC_H

extern u8 ascii2ebcdic_table[256];
extern u8 ebcdic2ascii_table[256];

/*
 * Generic translate buffer function.
 *
 * TODO: Use the TR instruction
 */
static inline void __translate(u8 *buf, int len, const u8 *table)
{
	int i;

	for(i=0;i<len;i++)
		buf[i] = table[buf[i]];
}

#define ascii2ebcdic(buf, len)	\
			__translate((buf), (len), ascii2ebcdic_table)

#endif
