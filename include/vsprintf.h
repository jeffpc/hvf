#ifndef __VSPRINTF_H
#define __VSPRINTF_H

/*
 * Pointers less than VSPRINTF_PAGE_SIZE will display as "<NULL>" if
 * displayed as %s
 */
#define VSPRINTF_PAGE_SIZE	4096

extern int snprintf(char *buf, int len, const char *fmt, ...)
        __attribute__ ((format (printf, 3, 4)));
extern int vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
        __attribute__ ((format (printf, 3, 0)));

#endif
