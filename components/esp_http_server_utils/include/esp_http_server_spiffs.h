#ifndef _ESP_HTTP_SERVER_SPIFFS_H_
#define _ESP_HTTP_SERVER_SPIFFS_H_

#include <stdbool.h>
#include <esp_err.h>
#include <esp_http_server.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief SPIFFS info handler, should be configured like:
 	httpd_uri_t upload_handler = {
		.uri       = "/spiffs_info",
		.method    = HTTP_GET,
		.handler   = esp_httpd_spiffs_info_handler,
	}
 *
 * @req The request being responded to
 *
 * @return
 *  - ESP_OK : On successful SPIFFS info request
 */
esp_err_t esp_httpd_spiffs_info_handler(httpd_req_t *req);

/**
 * @brief SPIFFS file upload handler, should be configured like:
 	httpd_uri_t upload_handler = {
		.uri       = "/file",
		.method    = HTTP_POST,
		.handler   = esp_httpd_spiffs_file_upload_handler,
		.user_ctx  = "/spiffs/upload"  //SPIFFS path for uploaded file
	}
 *
 * @req The request being responded to
 *
 * @return
 *  - ESP_OK : On successful file upload, or ESP_ERR otherwise
 */
esp_err_t esp_httpd_spiffs_file_upload_handler(httpd_req_t *req);

/**
 * @brief SPIFFS upload handler, should be configured like:

	esp_vfs_spiffs_conf_t spiffs_conf = {
		.base_path = "/spiffs",
		.partition_label = "storage",
		.max_files = 5,
		.format_if_mount_failed = true
	};
 	httpd_uri_t upload_handler = {
		.uri       = "/file",
		.method    = HTTP_POST,
		.handler   = esp_httpd_spiffs_image_upload_handler,
		.user_ctx  = spiffs_conf
	}
 *
 * @req The request being responded to
 *
 * @return
 *  - ESP_OK : On successful file upload, or ESP_ERR otherwise
 */
esp_err_t esp_httpd_spiffs_image_upload_handler(httpd_req_t *req);

#ifdef __cplusplus
}
#endif

#endif /* _ESP_HTTP_SERVER_SPIFFS_H_ */
