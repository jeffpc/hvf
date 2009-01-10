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

/**
 * strcmp - Compare two strings ignoring case
 * @s1: One string
 * @s2: Another string
 */
int strcasecmp(const char *s1, const char *s2)
{
        int c1, c2;

        do {
                c1 = tolower(*s1++);
                c2 = tolower(*s2++);
        } while (c1 == c2 && c1 != 0);
        return c1 - c2;
}

/**
 * strncpy - Copy a length-limited, %NUL-terminated string
 * @dest: Where to copy the string to
 * @src: Where to copy the string from
 * @count: The maximum number of bytes to copy
 *
 * The result is not %NUL-terminated if the source exceeds
 * @count bytes.
 *
 * In the case where the length of @src is less than  that  of
 * count, the remainder of @dest will be padded with %NUL.
 *
 */
char *strncpy(char *dest, const char *src, size_t count)
{
	char *tmp = dest;

	while (count) {
		if ((*tmp = *src) != 0)
			src++;
		tmp++;
		count--;
	}
	return dest;
}
