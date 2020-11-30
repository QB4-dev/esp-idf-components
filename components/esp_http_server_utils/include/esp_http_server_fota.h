#ifndef _ESP_HTTP_SERVER_FOTA_H_
#define _ESP_HTTP_SERVER_FOTA_H_

#include <esp_err.h>
#include <esp_http_server.h>

#include "esp_http_server_spiffs.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
	bool skip_reboot;
	void (*on_update_init)(void*);
	void (*on_update_done)(void*);
	void *arg;
}esp_ota_actions_t;

/**
 * @brief FOTA(Firmware Over The Air) update handler, should be configured like:
 *
	httpd_uri_t update_handler = {
		.uri       = "/update",
		.method    = HTTP_POST,
		.handler   = esp_httpd_fota_handler,
		.user_ctx  = &ota_actions //see below
	}

	Optional: It is possible to pass special actions before/after update as user_ctx:

	static esp_ota_actions_t ota_actions = {
		.skip_reboot = false,
		.on_update_init = on_init,
		.on_update_done = on_done,
		.arg = "FOTA_ARG"
	};
 *
 * @req The request being responded to
 *
 * @return
 *  - ESP_OK : On successful upgrade, error otherwise
 */
esp_err_t esp_httpd_fota_handler(httpd_req_t *req);

#ifdef __cplusplus
}
#endif

#endif /* _ESP_HTTP_SERVER_FOTA_H_ */
