/*
 * Copyright (c) 2007,2008 Josef 'Jeff' Sipek
 */

/**
 * strnlen - Find the length of a length-limited string
 * @s: The string to be sized
 * @count: The maximum number of bytes to search
 */
size_t strnlen(const char *s, size_t count)
{
        const char *sc;

        for (sc = s; count-- && *sc != '\0'; ++sc)
                /* nothing */;
        return sc - s;
}

/**
 * strcmp - Compare two strings
 * @cs: One string
 * @ct: Another string
 */
int strcmp(const char *cs, const char *ct)
{
	signed char __res;

	while (1) {
		if ((__res = *cs - *ct++) != 0 || !*cs++)
			break;
	}
	return __res;
}

/**
 * strncmp - Compare two strings
 * @cs: One string
 * @ct: Another string
 * @len: max length
 */
int strncmp(const char *cs, const char *ct, int len)
{
	signed char __res;

	while (1) {
		if ((__res = *cs - *ct++) != 0 || !*cs++ || len--)
			break;
	}
	return __res;
}
