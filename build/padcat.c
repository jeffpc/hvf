#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void cat(char *fn)
{
	FILE *f;
	char buf[80];
	size_t n;

	f = fopen(fn, "rb");
	if (!f) {
		fprintf(stderr, "Could not open file \"%s\".\n",
			fn);
		exit(1);
	}

	do {
		memset(buf, 0, 80);
		n = fread(buf, 1, 80, f);
		if (n)
			assert(fwrite(buf, 1, 80, stdout) == 80);
	} while(n == 80);

	fclose(f);
}

int main(int argc, char **argv)
{
	int i;

	for(i=1; i<argc; i++)
		cat(argv[i]);

	return 0;
}
