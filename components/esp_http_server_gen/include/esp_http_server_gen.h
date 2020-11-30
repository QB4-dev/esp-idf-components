#ifndef GENERATED_HANDLERS_H_
#define GENERATED_HANDLERS_H_

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include <esp_http_server.h>

typedef struct {
	const char *type;
	const uint8_t *data;
	size_t len;
}esp_httpd_content_t;

extern const httpd_uri_t generated_handlers[];
extern const size_t generated_handlers_num;

esp_err_t esp_httpd_send_content(httpd_req_t *req);

#endif /* GENERATED_HANDLERS_H_ */
