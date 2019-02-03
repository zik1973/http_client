#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http.h"
#include "log.h"

static int download_file(struct http_response *response, const char *output_file)
{
	FILE *file = fopen(output_file, "w");
	if (file == NULL) {
		error("Cann't open file '%s': %s errno=%d", output_file, strerror(errno), errno);
		return -1;
	}
	const char *transfer_encoding = http_response_get_header(response, "Transfer-Encoding");
	if (transfer_encoding && !strcasecmp(transfer_encoding, "chunked")) {
	}
	(void)transfer_encoding;
	const char *content_length = http_response_get_header(response, "Content-Length");
	(void)content_length;
	fclose(file);
	return 0;
}

int http_download(const char *url, const char *output_file)
{
	struct http_response response;
	int err = http_get(url, NULL, &response);
	info("%s", response.buf);
	if (!err && response.status_code == 200)
		err = download_file(&response, output_file);
	http_response_close(&response);
	return err;
}

#ifdef UNIT_TEST
void test_http_download(void)
{
	const char *home = getenv("HOME");
	char *file_name = aprintf("%s/test/http_client/web-control.ru.txt", home);
	assert(!http_download("http://web-control.ru/", file_name));
	free(file_name);
}
#endif
