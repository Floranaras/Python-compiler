#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * read_file() - Read a file's contents into a heap-allocated string.
 */
char *read_file(const char *filename)
{
	FILE *file;
	long size;
	char *buf;
	size_t nread;

	file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "error: cannot open '%s'\n", filename);
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);

	buf = malloc(size + 1);
	if (!buf) {
		fprintf(stderr,
			"error: out of memory reading '%s'\n", 
			filename);
		fclose(file);
		return NULL;
	}

	nread = fread(buf, 1, size, file);
	buf[nread] = '\0';

	fclose(file);
	return buf;
}
