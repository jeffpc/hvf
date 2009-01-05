static char* __extract_hex(char *str, u64 *val)
{
	u64 res;
	u64 tmp;
	int len;

	if (!str || !val)
		return ERR_PTR(-EINVAL);

	res = 0;
	len = -1;

	for (; str && *str != '\0'; str++, len++) {
		if (*str >= '0' && *str <= '9')
			tmp = *str - '0';
		else if (*str >= 'A' && *str <= 'F')
			tmp = *str - 'A' + 10;
		else if (*str >= 'a' && *str <= 'f')
			tmp = *str - 'a' + 10;
		else if (*str == ' ' || *str == '\t')
			break;
		else
			return ERR_PTR(-EINVAL);

		res = (res << 4) | tmp;
	}

	if (len == -1)
		return ERR_PTR(-EINVAL);

	*val = res;

	return str;
}

static char* __consume_ws(char *str)
{
	if (!str)
		return str;

	/* consume any extra whitespace */
	while(*str == ' ' || *str == '\t')
		str++;

	return str;
}
