/*
 * (C) Copyright 2007-2011  Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * This file is released under the GPLv2.  See the COPYING file for more
 * details.
 */

#include <ebcdic.h>
#include <lexer.h>

#include "config.tab.h"

int hex(char *a, char *b, u64 *out)
{
	u64 val;
	char c;

	if (a>=b)
		return 1;

	val = 0;

	while(a<b) {
		c = *a;

		if ((c >= '0') && (c <= '9'))
			val = (val << 4) | (*a - '0');
		else if ((c >= 'A') && (c <= 'F'))
			val = (val << 4) | (*a - 'A' + 10);
		else if ((c >= 'a') && (c <= 'f'))
			val = (val << 4) | (*a - 'a' + 10);
		else
			return 1;

		a++;
	}

	*out = val;

	return 0;
}

static int __fill(struct lexer *lexer, int n)
{
	char buf[CONFIG_LRECL];
	int end, i;
	int ret;

	if (unlikely(!lexer->init)) {
		/* look up the config file */
		lexer->file = edf_lookup(lexer->fs, CONFIG_FILE_NAME, CONFIG_FILE_TYPE);
		if (IS_ERR(lexer->file))
			return PTR_ERR(lexer->file);

		if ((lexer->file->FST.LRECL != CONFIG_LRECL) ||
		    (lexer->file->FST.RECFM != FSTDFIX))
			return -EINVAL;

		lexer->recoff = 0;
		lexer->buflen = sizeof(lexer->buf);
		lexer->filllen = 0;

		lexer->init = 1;
	}

	if (lexer->recoff) {
		lexer->filllen -= lexer->recoff;
		memmove(lexer->buf, &lexer->buf[lexer->recoff], lexer->filllen);
		lexer->recoff = 0;
	}

	assert(lexer->filllen < CONFIG_LRECL);

	ret = edf_read_rec(lexer->file, buf, lexer->recno++);
	if (ret)
		return ret;

	ebcdic2ascii((u8*) buf, CONFIG_LRECL);
	ascii2upper((u8*) buf, CONFIG_LRECL);

	/* find the last non-blank */
	for(end=CONFIG_LRECL-1; end>=0; end--)
		if (buf[end] != ' ')
			break;

	for(i=0; (i<CONFIG_LRECL) && (i<end+1); i++)
		lexer->buf[lexer->filllen+i] = buf[i];

	lexer->buf[lexer->filllen+i] = '\n';
	lexer->filllen += i+1;

	return 0;
}

#define YYFILL(n)	do { \
				int ret; \
				int off = cur - &lexer->buf[lexer->recoff]; \
				ret = __fill(lexer, (n)); \
				if (ret && !(lexer->filllen - lexer->recoff)) \
					return ret; \
				cur = &lexer->buf[off]; \
			} while(0)
#define YYRETURN(x)	do { \
				lexer->recoff = cur - lexer->buf; \
				return (x); \
			} while(0)
#define YYSKIP()	do { \
				lexer->recoff = cur - lexer->buf; \
				goto next; \
			} while(0)
int config_lex(void *data, void *yyval)
{
	struct lexer *lexer = data;
	YYSTYPE *val = yyval;
	char *cur;

next:
	cur = &lexer->buf[lexer->recoff];

	/*!re2c
		re2c:define:YYCTYPE = "char";
		re2c:define:YYCURSOR = cur;
		re2c:define:YYLIMIT = &lexer->buf[lexer->filllen];
		"\n"		{ YYRETURN(NLINE); }
		"*" [^\n]*	{ YYRETURN(COMMENT); }
		"OPERATOR"	{ YYRETURN(OPERATOR); }
		"RDEV"		{ YYRETURN(RDEV); }
		"LOGO"		{ YYRETURN(LOGO); }
		"CONSOLE"	{ YYRETURN(CONSOLE); }
		"USERID"	{ YYRETURN(USERID); }
		"LOCAL"		{ YYRETURN(LOCAL); }
		[0-9A-F]+	{ hex(&lexer->buf[lexer->recoff], cur, &val->num);
				  YYRETURN(NUM);
				}
		[A-Z]+		{ int len;
				  len = cur-&lexer->buf[lexer->recoff];
				  memcpy(lexer->retbuf, &lexer->buf[lexer->recoff], len);
				  lexer->retbuf[len] = '\0';
				  val->ptr = lexer->retbuf;
				  YYRETURN(WORD);
				}
		[ \t]+		{ YYSKIP(); }
	*/
}
