#include <assert.h>
#include <string.h>
#include <stdlib.h>
//#include <sys/types.h>
//#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include "http.h"
#include "url.h"

struct http_request {
	struct url	*url;
	const char	**headers;
	char		*body;
};

static int url_connect(struct url *url, int *sock)
{
	char *host = strndup(url->host, url->host_len);

	struct addrinfo hints = {
		.ai_flags = AI_ALL | AI_ADDRCONFIG,
		.ai_family = AF_UNSPEC, /* support both IPv4 and IPv6 */
		.ai_socktype = SOCK_STREAM
	};

	struct addrinfo *addrinfo = NULL;
	int err = getaddrinfo(host, "http", &hints, &addrinfo);
	if (err)
		return err;

	freeaddrinfo(addrinfo);
	return err;
}

int http_get(const char *url, const char **headers, struct http_response *response)
{
	struct url parsed;
	int err = url_parse(url, &parsed);
	if (err)
		return err;
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
	static const char url[] = "http://en.wikipedia.org/wiki/URL#Syntax";
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
