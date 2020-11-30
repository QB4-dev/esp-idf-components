#ifndef _ESP_HTTP_SERVER_WIFI_H_
#define _ESP_HTTP_SERVER_WIFI_H_

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include <esp_http_server.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t wifi_info_handler(httpd_req_t *req);
esp_err_t wifi_scan_handler(httpd_req_t *req);
esp_err_t wifi_connect_handler(httpd_req_t *req);

#ifdef __cplusplus
}
#endif

/**@}*/

#endif  // _ESP_HTTP_SERVER_WIFI_H_
