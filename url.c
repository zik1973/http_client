#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"
#include "url.h"

static int parse_scheme(const char **str, struct url *url)
{
	const char *start = *str;
	const char *colon = strchr(start, ':');
	if (colon == NULL)
		return ERR_URL_NO_SCHEME;
	url->scheme = start;
	url->scheme_len = colon - start;
	*str = colon + 1;
	return 0;
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

static int parse_port(const char *str, size_t len, struct url *url)
{
	if (len == 0)
		return 0;
	size_t port_len = 0;
	for (const char *ch = str; port_len < len && isdigit(*ch); port_len++, ch++)
		;
	if (port_len < len) {
		error("Invalid port value: '%.*s'", (unsigned int)len, str);
		return ERR_URL_INVALID_PORT;
	}
	url->port = str;
	url->port_len = port_len;
	return 0;
}

static int parse_authority(const char **str, struct url *parsed)
{
	const char *start = *str;
	/* if authority exists it starts from //, otherwise authority is empty */
	if (strncmp(start, "//", 2))
		return 0;
	start += 2;
	const char *slash = strchr(start, '/');
	size_t len = slash ? (slash - start) : strlen(start);
	const char *end = start + len;

	parse_userinfo(&start, &len, parsed);
	parsed->host = parse_host(&start, &len, &parsed->host_len);
	int err = parse_port(start, len, parsed);
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
	return *path_len ? start : NULL;
}

int url_parse(const char *url, struct url *parsed)
{
	memset(parsed, 0, sizeof(*parsed));
	int err = parse_scheme(&url, parsed);
	if (err)
		return err;
	if ((err = parse_authority(&url, parsed)))
		return err;
	parsed->path = parse_path(&url, &parsed->path_len);
	return 0;
}

#ifdef UNIT_TEST
static bool name_eq_null(const char *name, size_t name_len)
{
	if (name == NULL) {
		if (name_len == 0)
			return true;
		error("name == NULL && name_len != 0: name_len=%u", (unsigned int)name_len);
		return false;
	}
	error("name != NULL: name='%.*s'", (unsigned int)name_len, name);
	return false;
}

static bool name_eq(const char *name, size_t name_len, const char *value)
{
	if (value == NULL)
		return name_eq_null(name, name_len);

	size_t value_len = strlen(value);
	if (name_len != value_len) {
		error("name_len != value_len: name_len=%u, value_len=%u",
				(unsigned int)name_len, (unsigned int)value_len);
		return false;
	}
	if (strncmp(name, value, name_len)) {
		error("name != value: name='%.*s', value='%s'",
				(unsigned int)name_len, name, value);
		return false;
	}
	return true;
}

static void test_default(void)
{
	struct url parsed;
	assert(!url_parse("http://en.wikipedia.org/wiki/URL#Syntax", &parsed));

	assert(name_eq(parsed.scheme, parsed.scheme_len, "http"));
	assert(name_eq(parsed.username, parsed.username_len, NULL));
	assert(name_eq(parsed.password, parsed.password_len, NULL));
	assert(name_eq(parsed.host, parsed.host_len, "en.wikipedia.org"));
	assert(name_eq(parsed.port, parsed.port_len, NULL));
	assert(name_eq(parsed.path, parsed.path_len, "/wiki/URL"));
}

static void test_scheme_host(void)
{
	struct url parsed;
	assert(!url_parse("http://en.wikipedia.org", &parsed));

	assert(name_eq(parsed.scheme, parsed.scheme_len, "http"));
	assert(name_eq(parsed.username, parsed.username_len, NULL));
	assert(name_eq(parsed.password, parsed.password_len, NULL));
	assert(name_eq(parsed.host, parsed.host_len, "en.wikipedia.org"));
	assert(name_eq(parsed.port, parsed.port_len, NULL));
	assert(name_eq(parsed.path, parsed.path_len, ""));
}

static void test_scheme_host_port(void)
{
	struct url parsed;
	assert(!url_parse("http://en.wikipedia.org:8080", &parsed));

	assert(name_eq(parsed.scheme, parsed.scheme_len, "http"));
	assert(name_eq(parsed.username, parsed.username_len, NULL));
	assert(name_eq(parsed.password, parsed.password_len, NULL));
	assert(name_eq(parsed.host, parsed.host_len, "en.wikipedia.org"));
	assert(name_eq(parsed.port, parsed.port_len, "8080"));
	assert(name_eq(parsed.path, parsed.path_len, ""));
}

static void test_scheme_host_empty_port(void)
{
	struct url parsed;
	assert(!url_parse("http://en.wikipedia.org:", &parsed));

	assert(name_eq(parsed.scheme, parsed.scheme_len, "http"));
	assert(name_eq(parsed.username, parsed.username_len, NULL));
	assert(name_eq(parsed.password, parsed.password_len, NULL));
	assert(name_eq(parsed.host, parsed.host_len, "en.wikipedia.org"));
	assert(name_eq(parsed.port, parsed.port_len, NULL));
	assert(name_eq(parsed.path, parsed.path_len, ""));
}

static void test_scheme_host_non_numeric_port(void)
{
	struct url parsed;
	assert(url_parse("http://en.wikipedia.org:port", &parsed) == ERR_URL_INVALID_PORT);

	assert(name_eq(parsed.scheme, parsed.scheme_len, "http"));
	assert(name_eq(parsed.username, parsed.username_len, NULL));
	assert(name_eq(parsed.password, parsed.password_len, NULL));
	assert(name_eq(parsed.host, parsed.host_len, "en.wikipedia.org"));
	assert(name_eq(parsed.port, parsed.port_len, NULL));
	assert(name_eq(parsed.path, parsed.path_len, ""));
}

static void test_no_scheme_host(void)
{
	struct url parsed;
	assert(url_parse("en.wikipedia.org", &parsed) == ERR_URL_NO_SCHEME);
}

static void test_no_path(void)
{
	test_scheme_host();
	test_scheme_host_port();
	test_scheme_host_empty_port();
	test_scheme_host_non_numeric_port();
	test_no_scheme_host();
}

static void test_no_scheme_no_host_path(void)
{
	struct url parsed;
	assert(url_parse("/wiki/URL#Syntax", &parsed) == ERR_URL_NO_SCHEME);
}

void test_url_parse(void)
{
	test_default();
	test_no_path();
	test_no_scheme_no_host_path();
}
#endif
