#ifndef __STRING_H
#define __STRING_H

/*
 * NOTE! This ctype does not handle EOF like the standard C
 * library is required to. (Taken from include/linux/ctype.h)
 */

#define _U      0x01    /* upper */
#define _L      0x02    /* lower */
#define _D      0x04    /* digit */
#define _C      0x08    /* cntrl */
#define _P      0x10    /* punct */
#define _S      0x20    /* white space (space/lf/tab) */
#define _X      0x40    /* hex digit */
#define _SP     0x80    /* hard space (0x20) */

extern unsigned char _ascii_ctype[];

#define __ismask(x) (_ascii_ctype[(int)(unsigned char)(x)])

#define isalnum(c)      ((__ismask(c)&(_U|_L|_D)) != 0)
#define isalpha(c)      ((__ismask(c)&(_U|_L)) != 0)
#define iscntrl(c)      ((__ismask(c)&(_C)) != 0)
#define isdigit(c)      ((__ismask(c)&(_D)) != 0)
#define isgraph(c)      ((__ismask(c)&(_P|_U|_L|_D)) != 0)
#define islower(c)      ((__ismask(c)&(_L)) != 0)
#define isprint(c)      ((__ismask(c)&(_P|_U|_L|_D|_SP)) != 0)
#define ispunct(c)      ((__ismask(c)&(_P)) != 0)
#define isspace(c)      ((__ismask(c)&(_S)) != 0)
#define isupper(c)      ((__ismask(c)&(_U)) != 0)
#define isxdigit(c)     ((__ismask(c)&(_D|_X)) != 0)

/*
 * string.h equivalents
 */
#define memset(s,c,n)	__builtin_memset((s),(c),(n))
#define memcmp(d,s,l)	__builtin_memcmp((d),(s),(l))
#define memcpy(d,s,l)	__builtin_memcpy((d),(s),(l))
extern size_t strnlen(const char *s, size_t count);
extern int strcmp(const char *cs, const char *ct);
extern int strncmp(const char *cs, const char *ct, int len);
extern int strcasecmp(const char *s1, const char *s2);
extern char *strncpy(char *dest, const char *src, size_t count);

static inline unsigned char toupper(unsigned char c)
{
	/*
	 * TODO: This would break if we ever tried to compile within an EBCDIC
	 * environment
	 */
	if ((c >= 'a') && (c <= 'z'))
		c += 'A'-'a';
	return c;
}

static inline unsigned char tolower(unsigned char c)
{
	/*
	 * TODO: This would break if we ever tried to compile within an EBCDIC
	 * environment
	 */
	if ((c >= 'A') && (c <= 'Z'))
		c -= 'A'-'a';
	return c;
}


#endif
