#pragma once
#include "url.h"

struct http_response {
	unsigned char	http_version_major;
	unsigned char	http_version_minor;

	unsigned int	status_code;
	const char		*status_line;

	const char		**headers;

	int				socket;
	char			*buf;
	size_t			buf_size;
	char			*data;
	size_t			data_size;
	int				recv_errno;
	char			*header_buf;
};

/* Returns value of the HTTP header if found. Otherwise returns NULL. */
const char *http_response_get_header(struct http_response *response, const char *name);

int http_response_read(struct http_response *response, void *buf, size_t buf_len, size_t *data_size);

void http_response_close(struct http_response *response);

#define ERR_HTTP_URL_HAS_NO_HOST	-11
#define ERR_HTTP_SEND_FAILED		-12
#define ERR_HTTP_RECV_FAILED		-13
#define ERR_HTTP_BUFFER_TOO_SMALL	-14	/* buffer is not enough to obtain all HTTP headers */
#define ERR_HTTP_INVALID_RESPONSE	-15 /* Response header is not compliant to HTTP standard */

int http_get(const char *url, const char **headers, struct http_response *response);
int http_post(const char *url, const char **headers, const char *body, struct http_response *response);

int http_download(const char *url, const char *output_file);

#ifdef UNIT_TEST
void test_http_download(void);
void test_http(void);
#endif
