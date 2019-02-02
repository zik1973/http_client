#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "log.h"

void error(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

void info(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

char *aprintf(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	int size = vsnprintf(NULL, 0, format, ap);
	char *buffer = NULL;
	if (size <= 0)
		goto out;
	size_t bsize = size + 1;
	buffer = malloc(bsize);
	size = vsnprintf(buffer, bsize, format, ap);
	assert(size < bsize);
out:
	va_end(ap);
	return buffer;
}
