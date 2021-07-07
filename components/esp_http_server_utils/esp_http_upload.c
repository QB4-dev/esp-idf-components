#include "esp_http_upload.h"

#include <esp_http_server.h>
#include <esp_system.h>
#include <esp_log.h>
#include <sys/param.h>
#include <cJSON.h>

static const char *TAG="UPLOAD";

static bool get_boundary_str(const char *content, char *boundary)
{
	if(!content || !boundary)
		return false;

	/* expected content:
	Content-Type: multipart/form-data; boundary=---------------------------735323031399963166993862150 */
	if(!strstr(content,"multipart/form-data;"))
		return false;

	char *bound = strstr(content,"boundary=");
	if(!bound)
		return false;

	bound = strchr(bound,'=')+1; //move pointer to actual boundary
	if(bound[0]!='-' || bound[1]!='-')
		return false;

	snprintf(boundary,BOUNDARY_LEN,"--%s",bound); //additional "--"
		return true;
}

esp_err_t esp_http_get_boundary(httpd_req_t *req, char *boundary)
{
	esp_err_t rc;
	char *buf;
	size_t buf_len = httpd_req_get_hdr_value_len(req, "Content-Type");
	if(buf_len == 0)
		return ESP_ERR_NOT_FOUND;

	buf_len = buf_len+1; // one more byte for null terminator
	buf = malloc(buf_len);
	if(!buf)
		return ESP_ERR_NO_MEM;

	rc = httpd_req_get_hdr_value_str(req, "Content-Type", buf, buf_len);
	if(rc != ESP_OK){
		free(buf);
		return rc;
	}

	if(get_boundary_str(buf,boundary)){
		ESP_LOGD(TAG,"boundary=%s", boundary);
		rc = ESP_OK;
	} else {
		ESP_LOGE(TAG,"header boundary not found in: %s", buf);
		rc = ESP_ERR_INVALID_ARG;
	}

	free(buf);
	return rc;
}

int esp_http_upload_check_initial_boundary(httpd_req_t *req, char *boundary, size_t bytes_left)
{
	char buf[BOUNDARY_LEN] = {0};
	ssize_t  boundary_len = strlen(boundary);
	uint32_t bytes_read = 0;
	int32_t  recv;

	if(bytes_left < boundary_len)
		return -1;

	while(bytes_left > 0) {
		recv = httpd_req_recv(req, buf+bytes_read,(boundary_len-bytes_read));
		if(recv < 0) {
			if (recv == HTTPD_SOCK_ERR_TIMEOUT)
				continue;
			ESP_LOGE(TAG, "httpd_req_recv error: err=0x%d", recv);
			return -1;
		}
		bytes_read += recv;
		bytes_left -= recv;

		if(bytes_read == boundary_len)
			break;
	}

	if(strncmp(buf,boundary,strlen(boundary)) != 0){
		ESP_LOGE(TAG,"initial boundary not found");
		return -1;
	}
	return bytes_read;
}


int esp_http_upload_find_multipart_header_end(httpd_req_t *req, size_t bytes_left)
{
	const char seq[] = "\r\n\r\n";
	char buf[4] = {0};
	uint32_t bytes_read = 0;
	int32_t recv;
	uint8_t match = 0;

	//read byte by byte to find CRLF CRLF sequence
	while(bytes_left > 0) {
		recv = httpd_req_recv(req, buf, 1);
		if(recv < 0) {
			// Retry receive if timeout occurred
			if (recv == HTTPD_SOCK_ERR_TIMEOUT)
				continue;
			ESP_LOGE(TAG, "httpd_req_recv error: err=0x%d", recv);
			return -1;
		}
		bytes_read += recv;
		bytes_left -= recv;

		(buf[0] == seq[match]) ? (match++) : (match=0);
		if(match == 4)
			break; // CRLF CRLF sequence found
	}
	if(match != 4){
		ESP_LOGE(TAG, "CRLF CRLF seq not found");
		return -1;
	}
	return bytes_read;
}

//receive and check final boundary
int esp_http_upload_check_final_boundary(httpd_req_t *req, char *boundary, size_t bytes_left)
{
	char buf[BOUNDARY_LEN+2] = {0}; //additional '--' at the end
	ssize_t  boundary_len = strlen(boundary);
	uint32_t bytes_read = 0;
	int32_t recv;

	while(bytes_left > 0) {
		recv = httpd_req_recv(req, buf, MIN(bytes_left, UPLOAD_BUF_LEN));
		if(recv < 0) {
			// Retry receiving if timeout occurred
			if (recv == HTTPD_SOCK_ERR_TIMEOUT)
				continue;
			ESP_LOGE(TAG, "httpd_req_recv error: err=0x%d", recv);
			return -1;
		}
		bytes_read += recv;
		bytes_left -= recv;
	}
	/* Finally we should get something like:
	boundary:-----------------------------371500130728362684651932529351
	buf: CRLF-----------------------------371500130728362684651932529351--*/
	if(strncmp(buf+2,boundary,boundary_len-2) != 0){
		ESP_LOGE(TAG,"final boundary not found");
		return -1;
	}
	return bytes_read;
}

esp_err_t esp_http_upload_json_status(httpd_req_t *req, esp_err_t rc, int uploaded)
{
	cJSON *js = cJSON_CreateObject();
	if(!js)
		return ESP_ERR_NO_MEM;

	cJSON_AddStringToObject(js,"result",esp_err_to_name(rc));
	cJSON_AddNumberToObject(js,"bytes_uploaded",uploaded);

	char *js_txt = cJSON_Print(js);
	cJSON_Delete(js);

	httpd_resp_set_type(req,HTTPD_TYPE_JSON);
	httpd_resp_send(req, js_txt, -1);
	free(js_txt);
	return ESP_OK;
}
