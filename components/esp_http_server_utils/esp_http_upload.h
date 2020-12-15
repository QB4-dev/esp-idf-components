#ifndef _HTTPD_SERVER_UPLOAD_H_
#define _HTTPD_SERVER_UPLOAD_H_

#include <esp_err.h>
#include <esp_http_server.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define UPLOAD_BUF_LEN  2048
#define BOUNDARY_LEN    70   //as described in RFC1341

/**
 * @brief Find boundary string in request header
 *
 * @req The request being responded to
 * @boundary Pointer to boundary buffer(min 70bytes, see RFC1341)
 *
 * @return
 *  - ESP_OK : If boundary string has been found
 */
esp_err_t esp_http_get_boundary(httpd_req_t *req, char *boundary);

/**
 * @brief Find initial boundary string in request body
 *
 * @req The request being responded to
 * @boundary Pointer to boundary string
 * @bytes_left Request bytes left
 * @return bytes read, or -1 if not found
 */
int esp_http_upload_check_initial_boundary(httpd_req_t *req, char *boundary, size_t bytes_left);

/**
 * @brief Find multipart content header end(CRLFCRLF byte sequence)
 *
 * @req The request being responded to
 * @bytes_left Request bytes left
 * @return bytes read, or -1 if not found
 */
int esp_http_upload_find_multipart_header_end(httpd_req_t *req, size_t bytes_left);

/**
 * @brief Find final boundary string in request body
 *
 * @req The request being responded to
 * @boundary Pointer to boundary string
 * @bytes_left Request bytes left
 * @return bytes read, or -1 if not found
 */
int esp_http_upload_check_final_boundary(httpd_req_t *req, char *boundary, size_t bytes_left);

/**
 * @brief Return json upload status
 *
 * @req The request being responded to
 * @rc Upload result code
 * @bytuploaded es_left Request bytes left
 * @return ESP_OK or ESP_ERR_NO_MEM if json object cannot be created
 */
esp_err_t esp_http_upload_json_status(httpd_req_t *req, esp_err_t rc, int uploaded);

#ifdef __cplusplus
}
#endif

#endif /* _HTTPD_SERVER_UPLOAD_H_ */
