#pragma once
#include "url.h"

struct http_response {
	int status;
};

#define ERR_HTTP_URL_HAS_NO_HOST	-11

int http_get(const char *url, const char **headers, struct http_response *response);

void http_response_close(struct http_response *response);

#ifdef UNIT_TEST
void test_http(void);
#endif
