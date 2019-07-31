#ifndef __LEXER_L
#define __LEXER_L

#include <edf.h>

/* lexer state */
struct lexer {
	struct fs *fs;
	bool init;

	struct file *file;
	int recno;
	int recoff;

	int buflen;
	int filllen;
	char buf[CONFIG_LRECL*2];
	char retbuf[CONFIG_LRECL+1];
};

#endif
