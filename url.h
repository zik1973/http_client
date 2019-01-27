#pragma once
#include <stddef.h>

/* 
	https://en.wikipedia.org/wiki/URL#Syntax

	URI = scheme:[//authority]path[?query][#fragment]
	authority = [userinfo@]host[:port]
*/

struct url {
	const char *scheme;
	size_t scheme_len;
	const char *username;
	size_t username_len;
	const char *password;
	size_t password_len;
	const char *host;
	size_t host_len;
	const char *port;
	size_t port_len;
	const char *path;
	size_t path_len;
};

#define ERR_URL_INVALID_PORT	-1

int url_parse(const char *url,  struct url *parsed);

#ifdef UNIT_TEST
void test_url_parse();
#endif
