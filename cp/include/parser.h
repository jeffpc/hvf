#ifndef __PARSER_H
#define __PARSER_H

struct parser {
	/* memory related function for stack management */
	void*(*malloc)(size_t);
	void*(*realloc)(void*, size_t);
	void(*free)(void*);

	void(*error)(struct parser *, char*);

	int(*lex)(void *, void *);
	void *lex_data;
};

#endif
