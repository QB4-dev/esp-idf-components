#include <esp_http_server.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <esp_log.h>
#include <sys/param.h>

#include "include/esp_http_server_fota.h"
#include "esp_http_upload.h"

static const char *TAG="FOTA";

esp_err_t esp_httpd_fota_handler(httpd_req_t *req)
{
	esp_ota_handle_t update_handle;
	const esp_partition_t *update_partition;
	esp_ota_actions_t *ota_actions = req->user_ctx;
	esp_err_t ota_err;

	ESP_LOGI(TAG, "starting FOTA...");

	// do before update action
	if(ota_actions && ota_actions->on_update_init)
		ota_actions->on_update_init(ota_actions->arg);

	// prepare partition
	update_partition = esp_ota_get_next_update_partition(NULL);
	if (update_partition == NULL) {
		ESP_LOGE(TAG, "passive OTA partition not found");
		return ESP_FAIL;
	}
	ESP_LOGD(TAG, "write partition sub %d at offset 0x%x", update_partition->subtype, update_partition->address);

	ota_err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
	if(ota_err != ESP_OK) {
		ESP_LOGE(TAG, "esp_ota_begin failed: err=%d", ota_err);
		return ESP_FAIL;
	}

	ESP_LOGI(TAG, "esp_ota_begin OK");
	ESP_LOGI(TAG, "please wait...");

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

	//now we have content data until end boundary with additional '--' at the end
	ssize_t  boundary_len = strlen(boundary);
	ssize_t  final_boundary_len = boundary_len+2; // additional "--"
	uint32_t binary_size = bytes_left - (final_boundary_len+4); //CRLF CRLF
	uint32_t bytes_written = 0;
	uint32_t to_read;
	int32_t  recv;

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
			return ESP_FAIL;
		}
		bytes_left -= recv;

		// do here something with received data
		ota_err = esp_ota_write(update_handle, (const void *)buf, recv);
		if(ota_err != ESP_OK){
			ESP_LOGE(TAG, "httpd_req_recv error: err=0x%d", recv);
			free(buf);
			return ESP_FAIL;
		}
		bytes_written+=recv;
		ESP_LOGI(TAG,"firmware upload %d/%d bytes",bytes_written,binary_size);
	}
	free(buf);

	bytes_read = esp_http_upload_check_final_boundary(req,boundary,bytes_left);
	if(bytes_read > 0)
		bytes_left -= bytes_read;
	else
		return ESP_FAIL;

	ESP_LOGI(TAG,"%d firmware bytes uploaded OK",bytes_written);


	ota_err = esp_ota_end(update_handle);
	if (ota_err != ESP_OK) {
		ESP_LOGE(TAG, "esp_ota_end failed! err=0x%d. Image is invalid", ota_err);
		return ESP_FAIL;
	}

	ota_err = esp_ota_set_boot_partition(update_partition);
	if(ota_err != ESP_OK) {
		ESP_LOGE(TAG, "esp_ota_set_boot_partition failed! err=0x%d", ota_err);
		return ESP_FAIL;
	}

	//do after update action
	if(ota_actions && ota_actions->on_update_done)
		ota_actions->on_update_done(ota_actions->arg);

	httpd_resp_set_status(req, "200 OK");
	httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
	ESP_ERROR_CHECK(httpd_resp_send(req, "", -1));

	if(ota_actions && ota_actions->skip_reboot){
		ESP_LOGI(TAG, "reboot skipped");
	} else {
		ESP_LOGI(TAG, "esp reboot..");
		esp_restart();
	}
	return ESP_OK;
}
