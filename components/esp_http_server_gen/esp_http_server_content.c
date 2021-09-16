#include <esp_http_server.h>
#include <sys/param.h>
#include <esp_log.h>
#include <cJSON.h>

#include <esp_http_server_gen.h>

esp_err_t esp_httpd_send_content(httpd_req_t *req)
{
	esp_httpd_content_t *content = (esp_httpd_content_t*)req->user_ctx;

	httpd_resp_set_type(req,content->type);
	httpd_resp_send(req, (const char*)content->data, content->len);
	return ESP_OK;
}
