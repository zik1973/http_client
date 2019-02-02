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
	/*
	if (err == EAI_NODATA) {
		err = punycode(&host);
		assert(!err);
		err = getaddrinfo(host, "http", &hints, &addrinfo);
	}
	*/
	if (err)
		error("getaddrinfo(host='%s') failed: %s err=%d", host, gai_strerror(err), err);
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

struct http_headers {
	size_t	capacity;
	size_t	nr_headers;
	char	**headers;
};

static void http_headers_init(struct http_headers *headers, size_t capacity)
{
	assert(capacity > 0);
	headers->headers = calloc(capacity, sizeof(char*));
	assert(headers->headers);
	headers->capacity = capacity;
	headers->nr_headers = 0;
}

static void http_headers_term(struct http_headers *headers)
{
	free(headers->headers);
	memset(headers, 0, sizeof(headers));
}

static void http_headers_grow(struct http_headers *headers)
{
	size_t old_capacity = headers->capacity;
	size_t new_capacity = old_capacity * 2;
	headers->headers = realloc(headers->headers, new_capacity * sizeof(char*));
	assert(headers->headers);
	memset(headers->headers + old_capacity * sizeof(char*), 0,
		(new_capacity - old_capacity) * sizeof(char*));
	headers->capacity = new_capacity;
}

static const char *http_header_parse(const char *header, size_t *name_len)
{
	assert(!isspace(*header));

	const char *colon = strchr(header, ':');
	assert(colon);
	*name_len = colon - header;

	const char *value = colon + 1;

	/* Skip optional leading whitespace according to
	   https://tools.ietf.org/html/rfc7230#section-3.2 */
	while (isspace(*value))
		value++;

	return value;
}

static const char *http_header_is(const char *header, const char *name, size_t name_len)
{
	size_t name_len_2 = 0;
	const char *value = http_header_parse(header, &name_len_2);
	if (name_len != name_len_2)
		return NULL;
	if (strncasecmp(header, name, name_len))
		return NULL;
	return value;
}

static const char *http_header_get(struct http_headers *headers, const char *name, size_t name_len)
{
	for (size_t i = 0; i < headers->nr_headers; i++) {
		const char *value = http_header_is(headers->headers[i], name, name_len);
		if (value)
			return value;
	}
	return NULL;
}

#ifndef NDEBUG
static bool http_header_present(struct http_headers *headers, const char *header)
{
	size_t name_len = 0;
	http_header_parse(header, &name_len);
	return http_header_get(headers, header, name_len) != NULL;
}
#endif

static void http_header_add(struct http_headers *headers, const char *header)
{
	if (headers->nr_headers == headers->capacity)
		http_headers_grow(headers);
	assert(!http_header_present(headers, header));
	headers->headers[headers->nr_headers] = strdup(header);
	headers->nr_headers++;
}

static void http_header_set(struct http_headers *headers, const char *name,
							const char *value, size_t value_size)
{
	if (headers->nr_headers == headers->capacity)
		http_headers_grow(headers);
	assert(http_header_get(headers, name, strlen(name)) == NULL);
	headers->headers[headers->nr_headers] =
		aprintf("%s: %.*s", name, (unsigned int)value_size, value);
	headers->nr_headers++;
}

static void http_headers_set(struct http_headers *h, const char **headers)
{
	if (headers == NULL)
		return;
	for (size_t i = 0; headers[i]; i++)
		http_header_add(h, headers[i]);
}

struct http_request {
	const char			*method;
	const char			*url;
	struct url			parsed_url;
	struct http_headers	headers;
	const char			*body;
	int					socket;
};

static void http_request_init(struct http_request *request, const char *method)
{
	memset(request, 0, sizeof(*request));
	request->method = method;
	http_headers_init(&request->headers, 100);
	request->socket = -1;
}

static void http_request_term(struct http_request *request)
{
	if (request->socket != -1) {
		close(request->socket);
		request->socket = -1;
	}
	http_headers_term(&request->headers);
}

static void http_response_init(struct http_response *response)
{
	memset(response, 0, sizeof(*response));
	response->socket = -1;
}

void http_response_close(struct http_response *response)
{
	if (response->socket != -1) {
		close(response->socket);
		response->socket = -1;
	}
}

static void http_printf(struct buffer *buf, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	int size = vsnprintf(NULL, 0, format, ap);
	assert(size >= 0);
	buffer_reserve(buf, size);
	size_t space_len = buffer_space_len(buf);
	size = vsnprintf(buf->space, space_len, format, ap);
	assert(size >= 0);
	size_t usize = size;
	assert(usize < space_len);
	buf->space += usize;
	va_end(ap);
}

static int do_send(int s, const void *data, size_t len)
{
	ssize_t sent = send(s, data, len, 0);
	if (sent == -1) {
		error("send() failed: %s errno=%d", strerror(errno), errno);
		return ERR_HTTP_SEND_FAILED;
	}
	return 0;
}

static int http_send(struct http_request *request)
{
	assert(request->socket != -1);
	struct buffer buf;
	buffer_init(&buf, 1 << 12);

	http_printf(&buf, "%s %.*s HTTP/1.1\r\n", request->method,
		(unsigned int)request->parsed_url.path_len, request->parsed_url.path);
	for (size_t i = 0; i < request->headers.nr_headers; i++)
		http_printf(&buf, "%s\r\n", request->headers.headers[i]);
	http_printf(&buf, "\r\n");

	int err = do_send(request->socket, buf.data, buffer_data_len(&buf));
	buffer_term(&buf);
	if (err)
		return err;
	if (request->body)
		err = do_send(request->socket, request->body, strlen(request->body));
	return err;
}

static int http_recv(struct http_response *response)
{
	assert(response->socket != -1);
	size_t bsize = 1 << 20;
	char *buf = malloc(bsize);
	int err = 0;
	ssize_t read = recv(response->socket, buf, bsize, 0);
	if (read == -1) {
		error("recv() failed: %s errno=%d", strerror(errno), errno);
		err = ERR_HTTP_RECV_FAILED;
		goto out;
	}
	fprintf(stderr, "%.*s\n", (int)read, buf);

out:
	free(buf);
	return err;
}

/* Performs HTTP request-response transaction */
static int http_request_response(struct http_request *request, struct http_response *response)
{
	http_response_init(response);
	int err = url_parse(request->url, &request->parsed_url);
	if (err)
		return err;
	if (request->parsed_url.host_len == 0) {
		error("Could not connect to the url: '%s'. Host is empty.", request->url);
		return ERR_HTTP_URL_HAS_NO_HOST;
	}
	if (!http_header_get(&request->headers, "Host", 4)) {
		http_header_set(&request->headers, "Host",
			request->parsed_url.host, request->parsed_url.host_len);
	}
	if ((err = url_connect(&request->parsed_url, &request->socket)))
		return err;
	if ((err = http_send(request)))
		return err;
	response->socket = request->socket;
	request->socket = -1;
	return http_recv(response);
}

int http_get(const char *url, const char **headers, struct http_response *response)
{
	struct http_request request;
	http_request_init(&request, "GET");
	request.url = url;
	http_headers_set(&request.headers, headers);
	int err = http_request_response(&request, response);
	http_request_term(&request);
	return err;
}

int http_post(const char *url, const char **headers, const char *body, struct http_response *response)
{
	struct http_request request;
	http_request_init(&request, "POST");
	request.url = url;
	request.body = body;
	http_headers_set(&request.headers, headers);
	int err = http_request_response(&request, response);
	http_request_term(&request);
	return err;
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

static void test_free(void)
{
	char *null = NULL;
	free(null);
}

static void test_aprintf(void)
{
	int count = 4;
	const char word[] = "word";
	char *str = aprintf("sample %d %s", count, word);
	free(str);
}

static void test_tools(void)
{
	test_strndup();
	test_free();
	test_aprintf();
}

static void test_http_headers(void)
{
}

static void test_one(const char *url)
{
	struct http_response response;
	int err = http_get(url, NULL, &response);
	assert(!err);
	http_response_close(&response);
}

static void test_default(void)
{
	test_one("http://xn----dtbofgvdd5ah.xn--p1ai/4950985197/");
	return;
	test_one("http://кто-звонит.рф/4950985197/");
	test_one("http://en.wikipedia.org/wiki/URL#Syntax");
	test_one("http://yandex.ru/");
}

void test_http(void)
{
	test_tools();
	test_http_headers();
	test_default();
}
#endif
