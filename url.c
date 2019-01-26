#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "url.h"

static const char *parse_scheme(const char **str, size_t *scheme_len)
{
	const char *start = *str;
	*scheme_len = 0;
	const char *colon = strchr(start, ':');
	if (colon == NULL)
		return NULL;
	if (strncmp(colon, "://", 3))
		return NULL;
	*scheme_len = colon - start;
	*str = colon + 3;
	return start;
}

static void parse_userinfo(const char **str, size_t *len, struct url *url)
{
	const char *start = *str;
	const char *at = (const char*)memchr(start, '@', *len);
	if (at == NULL)
		return;
	size_t userinfo_len = at - start;
	*str = at + 1;
	*len -= userinfo_len + 1;
	
	const char *colon = (const char*)memchr(start, ':', userinfo_len);
	url->username = start;
	url->username_len = colon ? colon - start : userinfo_len;
	if (colon) {
		url->password = colon + 1;
		url->password_len = at - url->password;
	}
}

static const char *parse_host(const char **str, size_t *len, size_t *host_len)
{
	const char *start = *str;
	const char *colon = (const char*)memchr(start, ':', *len);
	if (colon) {
		*host_len = colon - start;
		*str = colon + 1;
		*len -= *host_len + 1;
	} else {
		*host_len = *len;
		*str = start + *host_len;
		*len = 0;
	}
	return start;
}

static int parse_port(const char *str, size_t len, unsigned short int *port)
{
	if (len == 0)
		return 0;
	char *end = NULL;
	long lport = strtol(str, &end, 10);
	if (str + len != end) {
		fprintf(stderr, "Invalid port value: '%*.s'", len, str);
		return ERR_URL_INVALID_PORT;
	}
	if (lport < 0) {
		fprintf(stderr, "Negative port value is invalid: %ld", lport);
		return ERR_URL_INVALID_PORT;
	}
	if (lport > 0xFFFF) {
		fprintf(stderr, "Port value could not be more than %ld, specified: %ld", 0xFFFFUL, lport);
		return ERR_URL_INVALID_PORT;
	}
	*port = (unsigned short int)lport;
	return 0;
}

static int parse_authority(const char **str, struct url *parsed)
{
	const char *start = *str;
	const char *slash = strchr(start, '/');
	size_t len = slash ? (slash - start) : strlen(start);
	const char *end = start + len;

	parse_userinfo(str, &len, parsed);
	parsed->host = parse_host(str, &len, &parsed->host_len);
	int err = parse_port(*str, len, &parsed->port);
	*str = end;
	return err;
}

static const char *parse_path(const char **str, size_t *path_len)
{
	const char *start = *str;
	*path_len = 0;
	const char *hash = strchr(start, '#');
	*path_len = hash ? hash - start : strlen(start);
	*str = start + *path_len;
	return start;
}

int url_parse(const char *url, struct url *parsed)
{
	memset(parsed, 0, sizeof(*parsed));
	parsed->scheme = parse_scheme(&url, &parsed->scheme_len);
	int err = parse_authority(&url, parsed);
	if (err)
		return err;
	parsed->path = parse_path(&url, &parsed->path_len);
	return 0;
}

#ifdef UNIT_TEST
static void	test_default()
{
	struct url parsed;
	assert(!url_parse("http://en.wikipedia.org/wiki/URL#Syntax", &parsed));

	assert(parsed.scheme_len == 4);
	assert(!memcmp(parsed.scheme, "http", 4));

	assert(parsed.username == NULL);
	assert(parsed.username_len == 0);

	assert(parsed.password == NULL);
	assert(parsed.password_len == 0);

	static const char host[] = "en.wikipedia.org";
	size_t host_len = strlen(host);
	assert(parsed.host_len == host_len);
	assert(!memcmp(parsed.host, host, host_len));

	assert(parsed.port == 0);

	static const char path[] = "/wiki/URL";
	size_t path_len = strlen(path);
	assert(parsed.path_len == path_len);
	assert(!memcmp(parsed.path, path, path_len));
}

void test_url_parse()
{
	test_default();
}

int main()
{
	test_url_parse();
	return 0;
}
#endif
