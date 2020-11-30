#include "include/esp_http_server_spiffs.h"

#include <esp_http_server.h>
#include <esp_system.h>
#include <esp_spiffs.h>
#include <esp_log.h>
#include <sys/param.h>
#include <cJSON.h>

#include "esp_http_upload.h"

static const char *TAG="SPIFFS";

esp_err_t esp_httpd_spiffs_info_handler(httpd_req_t *req)
{
	size_t total = 0, used = 0;
	esp_err_t rc = esp_spiffs_info(NULL, &total, &used);
	cJSON* js = cJSON_CreateObject();
	if(!js)
		return ESP_FAIL;

	cJSON_AddStringToObject(js,"result",esp_err_to_name(rc));
	cJSON_AddNumberToObject(js,"used",used);
	cJSON_AddNumberToObject(js,"total",total);

	char *js_txt = cJSON_Print(js);
	cJSON_Delete(js);
	if(!js_txt)
		return ESP_FAIL;

	ESP_ERROR_CHECK(httpd_resp_set_type(req,HTTPD_TYPE_JSON));
	ESP_ERROR_CHECK(httpd_resp_send(req, js_txt, -1));
	free(js_txt);
	return ESP_OK;
}

esp_err_t esp_httpd_spiffs_upload_handler(httpd_req_t *req)
{
	char boundary[BOUNDARY_LEN] = {0};
	size_t   bytes_left = req->content_len;
	int32_t  bytes_read = 0;
	esp_err_t rc;

	rc = esp_http_get_boundary(req,boundary);
	if( rc != ESP_OK)
		return rc;

	//Read request data
	bytes_read = esp_http_upload_check_initial_boundary(req,boundary,bytes_left);
	if(bytes_read > 0)
		bytes_left -= bytes_read;
	else
		return ESP_ERR_INVALID_ARG;

	bytes_read = esp_http_upload_find_multipart_header_end(req,bytes_left);
	if(bytes_read > 0)
		bytes_left -= bytes_read;
	else
		return ESP_ERR_INVALID_ARG;

	const char *upload_path = req->user_ctx;
	if(!upload_path){
		ESP_LOGE(TAG, "upload path not found");
		return ESP_ERR_INVALID_ARG;
	}

	ESP_LOGI(TAG, "opening file %s",upload_path);
	FILE* f = fopen(upload_path, "w");
	if (f == NULL) {
		ESP_LOGE(TAG, "Failed to open file for writing");
		return ESP_FAIL;
	}

	//now we have content data until end boundary
	ssize_t  boundary_len = strlen(boundary);
	ssize_t  final_boundary_len = boundary_len+2; // additional "--"
	uint32_t binary_size = bytes_left - (final_boundary_len+4); //CRLF CRLF
	uint32_t bytes_written = 0;
	uint32_t to_read;
	int32_t  recv;
	size_t wr;

	//prepare buffer, keep in mind to free it before return call
	char *buf = (char *)malloc(UPLOAD_BUF_LEN);
	if(!buf)
		return ESP_ERR_NO_MEM;

	while(bytes_written < binary_size) {
		if(bytes_left > UPLOAD_BUF_LEN)
			to_read = MIN(bytes_left, UPLOAD_BUF_LEN);
		else
			to_read = bytes_left - (final_boundary_len+4);

		recv = httpd_req_recv(req, buf, to_read);
		if(recv < 0) {
			// Retry receiving if timeout occurred
			if (recv == HTTPD_SOCK_ERR_TIMEOUT)
				continue;
			ESP_LOGE(TAG, "httpd_req_recv error: err=0x%d", recv);
			free(buf);
			fclose(f);
			return ESP_FAIL;
		}
		bytes_left -= recv;

		wr = fwrite((const void *)buf, recv, 1,f);
		if(wr != 1){
			ESP_LOGE(TAG, "spiffs write error: err=0x%d", wr);
			free(buf);
			fclose(f);
			return ESP_FAIL;
		}
		bytes_written+=recv;
		ESP_LOGI(TAG,"file upload %d/%d bytes",bytes_written,binary_size);
	}
	free(buf);
	fclose(f);

	bytes_read = esp_http_upload_check_final_boundary(req,boundary,bytes_left);
	if(bytes_read > 0)
		bytes_left -= bytes_read;
	else
		return ESP_FAIL;

	ESP_LOGI(TAG,"%d file bytes uploaded OK",bytes_written);

	httpd_resp_set_status(req, "200 OK");
	httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
	ESP_ERROR_CHECK(httpd_resp_send(req, "", -1));
	return ESP_OK;
}

