#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include "http.h"
#include "log.h"
#include "url.h"

struct http_request {
	struct url	*url;
	const char	**headers;
	char		*body;
};

static int url_connect(struct url *url, int *sock)
{
	assert(url->host && url->host_len);
	char *host = strndup(url->host, url->host_len);

	/* TODO: Add HTTPS support */
	assert(url->scheme_len == 4);
	assert(!strncasecmp(url->scheme, "http", 4));

	struct addrinfo hints = {
		.ai_flags = AI_ALL | AI_ADDRCONFIG,
		//.ai_flags = AI_ALL,
		.ai_family = AF_UNSPEC, /* support both IPv4 and IPv6 */
		.ai_socktype = SOCK_STREAM
	};

	struct addrinfo *addrinfo = NULL;
	int err = getaddrinfo(host, "http", &hints, &addrinfo);
	free(host);
	if (err)
		return err;

	for (struct addrinfo *cur = addrinfo; cur != NULL; cur = cur->ai_next) {
		int s = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
		if (s == -1) {
			error("socket() failed: %s, err=%d", strerror(errno), errno);
			continue;
		}
		if (connect(s, cur->ai_addr, cur->ai_addrlen)) {
			error("connect() failed: %s, err=%d", strerror(errno), errno);
			close(s);
			continue;
		}
		info("connect successfull");
		*sock = s;
		break;
	}

	freeaddrinfo(addrinfo);
	return err;
}

int http_get(const char *url, const char **headers, struct http_response *response)
{
	struct url parsed;
	int err = url_parse(url, &parsed);
	if (err)
		return err;
	if (parsed.host_len == 0) {
		error("Could not connect to the url: '%s'. Host is empty.", url);
		return ERR_HTTP_URL_HAS_NO_HOST;
	}
	int sock = -1;
	if ((err = url_connect(&parsed, &sock)))
		return err;
	close(sock);
	return 0;
}

void http_response_close(struct http_response *response)
{
}

#ifdef UNIT_TEST
static void test_strndup(void)
{
	const char *null = NULL;
	char *dup = strndup(null, 0);
	assert(dup != NULL);
	assert(*dup == 0);
	free(dup);
}

static void test_default(void)
{
	//static const char url[] = "http://en.wikipedia.org/wiki/URL#Syntax";
	static const char url[] = "http://yandex.ru";
	struct http_response response;
	int err = http_get(url, NULL, &response);
	assert(!err);
	http_response_close(&response);
}

void test_http(void)
{
	test_strndup();
	test_default();
}
#endif
