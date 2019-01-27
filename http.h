#pragma once
#include "url.h"

struct http_response {
	int socket;
	int status;
};

void http_response_close(struct http_response *response);

#define ERR_HTTP_URL_HAS_NO_HOST	-11
#define ERR_HTTP_SEND_FAILED		-12
#define ERR_HTTP_RECV_FAILED		-13

int http_get(const char *url, const char **headers, struct http_response *response);
int http_post(const char *url, const char **headers, const char *body, struct http_response *response);

#ifdef UNIT_TEST
void test_http(void);
#endif
