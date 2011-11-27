#ifndef __UTIL_H
#define __UTIL_H

extern char *strdup(char *s, int flags);
extern int hex(char *a, char *b, u64 *out);
extern int bcd2dec(u64 val, u64 *out);

#endif
