/*
 * server.c
 *
 *  Created on: 7 lis 2020
 *      Author: kuba
 */

#include "server.h"

#include <esp_log.h>
#include <esp_system.h>
#include <esp_http_server_gen.h>
#include <esp_http_server_wifi.h>
#include <esp_http_server_fota.h>

static const char *TAG="SRV";

static httpd_uri_t info_handler = {
	.uri       = "/info",
	.method    = HTTP_GET,
	.handler   = wifi_info_handler,
	.user_ctx  = NULL
};

static httpd_uri_t spiffs_handler = {
	.uri       = "/spiffs",
	.method    = HTTP_GET,
	.handler   = esp_httpd_spiffs_info_handler,
	.user_ctx  = NULL
};

static httpd_uri_t scan_handler = {
	.uri       = "/scan",
	.method    = HTTP_GET,
	.handler   = wifi_scan_handler,
	.user_ctx  = NULL
};

static httpd_uri_t connect_handler = {
	.uri       = "/connect",
	.method    = HTTP_POST,
	.handler   = wifi_connect_handler,
	.user_ctx  = NULL
};

static void on_update_init(void *arg)
{
	ESP_LOGI(TAG, "ON UPGRADE INIT HANDLER with arg: %s",(char*)arg);
}

static void on_update_complete(void *arg)
{
	ESP_LOGI(TAG, "ON UPGRADE complete HANDLER with arg: %s",(char*)arg);
}

static esp_ota_actions_t actions = {
	.skip_reboot = false,
	.on_update_init = on_update_init,
	.on_update_complete = on_update_complete,
	.arg = "FOTA_ARG"
};

static httpd_uri_t fota_handler = {
	.uri       = "/upgrade",
	.method    = HTTP_POST,
	.handler   = esp_httpd_fota_handler,
	.user_ctx  = &actions
};

httpd_handle_t start_webserver(void)
{
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	esp_err_t rc;

	config.max_uri_handlers = 32;

	// Start the httpd server
	ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
	ESP_LOGI(TAG, "Free heap size: '%d'", esp_get_free_heap_size());
	rc = httpd_start(&server, &config);
	if (rc == ESP_OK) {
		// Set URI handlers
		ESP_LOGI(TAG, "Registering URI handlers");
		ESP_ERROR_CHECK(httpd_register_uri_handler(server, &info_handler));
		ESP_ERROR_CHECK(httpd_register_uri_handler(server, &scan_handler));
		ESP_ERROR_CHECK(httpd_register_uri_handler(server, &spiffs_handler));
		ESP_ERROR_CHECK(httpd_register_uri_handler(server, &connect_handler));
		ESP_ERROR_CHECK(httpd_register_uri_handler(server, &fota_handler));
		for(int i=0; i < generated_handlers_num; i++){
			ESP_ERROR_CHECK(httpd_register_uri_handler(server, &generated_handlers[i]));
			ESP_LOGI(TAG, "registered new URI: %s", generated_handlers[i].uri);
		}
		ESP_LOGI(TAG, "Free heap size: '%d'", esp_get_free_heap_size());
		return server;
	}

	ESP_LOGI(TAG, "Error starting server! %s", esp_err_to_name(rc));
	return NULL;
}

void stop_webserver(httpd_handle_t server)
{
	httpd_stop(server);
}
