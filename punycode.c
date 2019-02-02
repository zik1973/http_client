#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include "buffer.h"
#include "http.h"
#include "log.h"
#include "url.h"

/* Convert hostname from UTF-8 to Punycode according to https://tools.ietf.org/html/rfc3492 */
int punycode(char **host)
{
	char *idna = *host; /* Internationalized Domain Name in Applications */
	size_t idna_len = strlen(idna);

	char *ascii = malloc(idna_len * 3);
	char *end_ascii = ascii + idna_len * 3;

	/* First, all basic ASCII characters in the string are copied from input to output,
	   skipping over any other characters. */
	char *dest = ascii;
	size_t non_ascii_cnt = 0;
	for (const char *src = idna; *src; src++) {
		if (((signed char)*src) > 0)
			*dest++ = *src;
		else
			non_ascii_cnt++;
	}

	if (non_ascii_cnt == 0) {
		free(ascii);
		return 0;
	}

	/* If any characters were copied an ASCII hyphen is added to the output next. */
	if (dest > ascii) {
			assert(dest < end_ascii);
			*dest++ = '-';
	}

	/* Not implemented yet */
	assert(0);

	free(idna);
	*host = ascii;
	return 0;
}

#ifdef UNIT_TEST
static int test_punycode_one(const char *idna, const char *result)
{
	char *host = strdup(idna);
	int err = punycode(&host);
	if (!err)
		assert(!strcmp(host, result));
	free(host);
	return err;
}

void test_punycode(void)
{
	/* Punycode is not implemented yet */
	return;
	assert(!test_punycode_one("bücher", "bcher-kva"));
	assert(!test_punycode_one("München", "Mnchen-3ya"));
	assert(!test_punycode_one("кто-звонит.рф", "xn----dtbofgvdd5ah.xn--p1ai"));
}
#endif
