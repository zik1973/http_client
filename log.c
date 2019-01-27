#include <stdarg.h>
#include <stdio.h>
#include "log.h"

void err(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}
