#include "include/esp_http_server_spiffs.h"

#include <esp_http_server.h>
#include <esp_system.h>
#include <esp_spiffs.h>
#include <esp_partition.h>
#include <esp_log.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cJSON.h>

#include "esp_http_upload.h"

static const char *TAG="SPIFFS";

static cJSON *spiffs_file_list_to_json(const char *path)
{
	struct dirent* de;
	DIR* dir = opendir(path);
	if(!dir)
		return NULL;

	cJSON *js_array = cJSON_CreateArray();
	if(!js_array){
		closedir(dir);
		return NULL;
	}

	while(true) {
		de = readdir(dir);
		if (!de)
			break;
		cJSON_AddItemToArray(js_array, cJSON_CreateString(de->d_name));
	}
	closedir(dir);
	return js_array;
}

esp_err_t esp_httpd_spiffs_info_handler(httpd_req_t *req)
{
	char*  buf;
	size_t buf_len;
	size_t total = 0, used = 0;
	esp_err_t rc = ESP_OK;

	const esp_vfs_spiffs_conf_t *esp_vfs_spiffs_conf = req->user_ctx;
	if(!esp_vfs_spiffs_conf){
		ESP_LOGE(TAG, "esp_vfs_spiffs_conf not found");
		return ESP_ERR_INVALID_ARG;
	}

	buf_len = httpd_req_get_url_query_len(req) + 1;
	if(buf_len > 1) {
		buf = malloc(buf_len);
		if(!buf)
			return ESP_ERR_NO_MEM;

		if(httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
			char param[32];
			if(httpd_query_key_value(buf, "remove", param, sizeof(param)) == ESP_OK) {
				ESP_LOGI(TAG, "found URL query parameter -> remove=%s", param);
				rc = unlink(param);
			}
		}
		free(buf);
	}

	cJSON* js = cJSON_CreateObject();
	if(!js)
		return ESP_FAIL;

	esp_spiffs_info(esp_vfs_spiffs_conf->partition_label, &total, &used);
	cJSON_AddStringToObject(js,"result",esp_err_to_name(rc));
	cJSON_AddNumberToObject(js,"used",used);
	cJSON_AddNumberToObject(js,"total",total);
	cJSON_AddNumberToObject(js,"percent",100*used/total);
	cJSON_AddItemToObject(js,"files",spiffs_file_list_to_json(esp_vfs_spiffs_conf->base_path));

	char *js_txt = cJSON_Print(js);
	cJSON_Delete(js);
	if(!js_txt)
		return ESP_FAIL;

	ESP_ERROR_CHECK(httpd_resp_set_type(req,HTTPD_TYPE_JSON));
	ESP_ERROR_CHECK(httpd_resp_send(req, js_txt, -1));
	free(js_txt);
	return ESP_OK;
}

esp_err_t esp_httpd_spiffs_file_upload_handler(httpd_req_t *req)
{
	char boundary[BOUNDARY_LEN] = {0};
	size_t   bytes_left = req->content_len;
	int32_t  bytes_read = 0;
	esp_err_t rc;

	const char *upload_path = req->user_ctx;
	if(!upload_path){
		ESP_LOGE(TAG, "upload path not found");
		return esp_http_upload_json_status(req,ESP_ERR_INVALID_ARG,0);
	}

	rc = esp_http_get_boundary(req,boundary);
	if( rc != ESP_OK)
		return esp_http_upload_json_status(req,rc,0);

	//Read request data
	bytes_read = esp_http_upload_check_initial_boundary(req,boundary,bytes_left);
	if(bytes_read > 0)
		bytes_left -= bytes_read;
	else
		return esp_http_upload_json_status(req,ESP_ERR_INVALID_ARG,0);

	bytes_read = esp_http_upload_find_multipart_header_end(req,bytes_left);
	if(bytes_read > 0)
		bytes_left -= bytes_read;
	else
		return esp_http_upload_json_status(req,ESP_ERR_INVALID_ARG,0);

	ESP_LOGI(TAG, "opening file %s",upload_path);
	FILE* f = fopen(upload_path, "w");
	if (f == NULL) {
		ESP_LOGE(TAG, "Failed to open file for writing");
		return esp_http_upload_json_status(req,ESP_FAIL,0);
	}

	//now we have content data until end boundary
	ssize_t  boundary_len = strlen(boundary);
	ssize_t  final_boundary_len = boundary_len+2; // additional "--"
	uint32_t binary_size = bytes_left - (final_boundary_len+4); //CRLF CRLF
	uint32_t bytes_written = 0;
	uint32_t to_read;
	int32_t  recv;
	size_t   wr;

	if(binary_size == 0){
		fclose(f);
		ESP_LOGE(TAG, "no file uploaded");
		return esp_http_upload_json_status(req,ESP_ERR_NOT_FOUND,0);
	}

	//prepare buffer, keep in mind to free it before return call
	char *buf = (char *)malloc(UPLOAD_BUF_LEN);
	if(!buf){
		fclose(f);
		return esp_http_upload_json_status(req,ESP_ERR_NO_MEM,0);
	}

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
			return esp_http_upload_json_status(req,ESP_FAIL,bytes_written);
		}
		bytes_left -= recv;

		wr = fwrite((const void *)buf, recv, 1,f);
		if(wr != 1){
			ESP_LOGE(TAG, "spiffs write error: err=0x%d", wr);
			free(buf);
			fclose(f);
			return esp_http_upload_json_status(req,ESP_FAIL,bytes_written);
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
		return esp_http_upload_json_status(req,ESP_FAIL,bytes_written);

	ESP_LOGI(TAG,"%d file bytes uploaded OK",bytes_written);
	return esp_http_upload_json_status(req,ESP_OK,bytes_written);
}

esp_err_t esp_httpd_spiffs_image_upload_handler(httpd_req_t *req)
{
	char boundary[BOUNDARY_LEN] = {0};
	size_t   bytes_left = req->content_len;
	int32_t  bytes_read = 0;
	esp_err_t rc;

	const esp_vfs_spiffs_conf_t *esp_vfs_spiffs_conf = req->user_ctx;
	if(!esp_vfs_spiffs_conf){
		ESP_LOGE(TAG, "esp_vfs_spiffs_conf not found");
		return esp_http_upload_json_status(req,ESP_ERR_INVALID_ARG,0);
	}

	const char *label = esp_vfs_spiffs_conf->partition_label;
	if(!label){
		ESP_LOGE(TAG, "partition label not set");
		return esp_http_upload_json_status(req,ESP_ERR_INVALID_ARG,0);
	}

	const esp_partition_t *spiffs_part;
	spiffs_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_SPIFFS,label);
	if(!spiffs_part){
		ESP_LOGE(TAG, "partition %s not found",label);
		return esp_http_upload_json_status(req,ESP_ERR_NOT_FOUND,0);
	}

	spiffs_part = esp_partition_verify(spiffs_part);
	if(!spiffs_part){
		ESP_LOGE(TAG, "partition verify failed");
		return esp_http_upload_json_status(req,ESP_FAIL,0);
	}

	rc = esp_http_get_boundary(req,boundary);
	if( rc != ESP_OK)
		return esp_http_upload_json_status(req,rc,0);

	//Read request data
	bytes_read = esp_http_upload_check_initial_boundary(req,boundary,bytes_left);
	if(bytes_read > 0)
		bytes_left -= bytes_read;
	else
		return esp_http_upload_json_status(req,ESP_ERR_INVALID_ARG,0);

	bytes_read = esp_http_upload_find_multipart_header_end(req,bytes_left);
	if(bytes_read > 0)
		bytes_left -= bytes_read;
	else
		return esp_http_upload_json_status(req,ESP_ERR_INVALID_ARG,0);

	//now we have content data until end boundary
	ssize_t  boundary_len = strlen(boundary);
	ssize_t  final_boundary_len = boundary_len+2; // additional "--"
	uint32_t binary_size = bytes_left - (final_boundary_len+4); //CRLF CRLF
	uint32_t bytes_written = 0;
	uint32_t to_read;
	int32_t  recv;

	if(binary_size == 0){
		ESP_LOGE(TAG, "no file uploaded");
		return esp_http_upload_json_status(req,ESP_ERR_NOT_FOUND,0);
	}

	if(binary_size > spiffs_part->size){
		ESP_LOGE(TAG, "image file too big");
		return esp_http_upload_json_status(req,ESP_ERR_INVALID_SIZE,0);
	}

	ESP_LOGI(TAG, "\"%s\" partition found, formating...",label);

	if(esp_spiffs_mounted(label))
		esp_vfs_spiffs_unregister(label);

	rc = esp_partition_erase_range(spiffs_part, 0, spiffs_part->size);
	if(rc != ESP_OK){
		ESP_LOGE(TAG, "partition erase failed: err=0x%d",rc);
		return esp_http_upload_json_status(req,ESP_FAIL,0);
	}

	//prepare buffer, keep in mind to free it before return call
	char *buf = (char *)malloc(UPLOAD_BUF_LEN);
	if(!buf)
		return esp_http_upload_json_status(req,ESP_ERR_NO_MEM,0);

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
			return esp_http_upload_json_status(req,ESP_FAIL,bytes_written);
		}
		bytes_left -= recv;

		rc = esp_partition_write(spiffs_part,bytes_written,(const void *)buf,recv);
		if(rc != ESP_OK){
			ESP_LOGE(TAG, "image write error: err=0x%d", rc);
			free(buf);
			return esp_http_upload_json_status(req,rc,bytes_written);
		}
		bytes_written+=recv;
		ESP_LOGI(TAG,"image upload %d/%d bytes",bytes_written,binary_size);
	}
	free(buf);

	bytes_read = esp_http_upload_check_final_boundary(req,boundary,bytes_left);
	if(bytes_read > 0)
		bytes_left -= bytes_read;
	else
		return esp_http_upload_json_status(req,ESP_FAIL,bytes_written);

	ESP_LOGI(TAG,"image upload complete %d bytes uploaded OK",bytes_written);
	rc = esp_vfs_spiffs_register(esp_vfs_spiffs_conf);
	if(rc != ESP_OK){
		ESP_LOGE(TAG, "esp_vfs_spiffs_register error: err=0x%d", rc);
		return esp_http_upload_json_status(req,rc,bytes_written);
	}
	ESP_LOGI(TAG,"esp_vfs_spiffs registered OK");
	return esp_http_upload_json_status(req,ESP_OK,bytes_written);
}

