#include <esp_http_server.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <tcpip_adapter.h>
#include <sys/param.h>
#include <cJSON.h>

#include "include/esp_http_server_wifi.h"

#define DEFAULT_SCAN_LIST_SIZE 16

static const char *TAG="WiFi";

static cJSON *ap_record_to_json(wifi_ap_record_t *ap_info)
{
	char mac_buf[24];
	cJSON *js = cJSON_CreateObject();
	if(!js)
		return NULL;

	sprintf(mac_buf,MACSTR,MAC2STR(ap_info->bssid));

	cJSON_AddStringToObject(js, "bssid",mac_buf);
	cJSON_AddStringToObject(js, "ssid", (const char *)ap_info->ssid);
	cJSON_AddNumberToObject(js, "rssi", (const double)ap_info->rssi);
	cJSON_AddStringToObject(js, "authmode",
			ap_info->authmode == WIFI_AUTH_OPEN ? "OPEN" :
			ap_info->authmode == WIFI_AUTH_WEP ? "WEP" :
			ap_info->authmode == WIFI_AUTH_WPA_PSK ? "WPA_PSK" :
			ap_info->authmode == WIFI_AUTH_WPA2_PSK ? "WPA2_PSK" :
			ap_info->authmode == WIFI_AUTH_WPA_WPA2_PSK ? "WPA_WPA2_PSK" :
			ap_info->authmode == WIFI_AUTH_WPA2_ENTERPRISE ? "WPA2_ENTERPRISE" :
	"??");
	return js;
}

static cJSON *ip_info_to_json(tcpip_adapter_ip_info_t *ip_info)
{
	char buf[16];
	cJSON *js = cJSON_CreateObject();

	if(!js)
		return NULL;

	sprintf(buf,IPSTR,IP2STR(&ip_info->ip));
	cJSON_AddStringToObject(js, "ip", buf);

	sprintf(buf,IPSTR,IP2STR(&ip_info->netmask));
	cJSON_AddStringToObject(js, "netmask", buf);

	sprintf(buf,IPSTR,IP2STR(&ip_info->gw));
	cJSON_AddStringToObject(js, "gw", buf);
	return js;
}

/* get wifi access point interface info */
static cJSON *get_wifi_ap_info(void)
{
	tcpip_adapter_ip_info_t ip_info = {0};

	cJSON *js = cJSON_CreateObject();
	if(!js)
		return NULL;

	if(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP,&ip_info) == ESP_OK)
		cJSON_AddItemToObject(js,"ip_info",ip_info_to_json(&ip_info));
	return js;
}

/* get wifi station interface info */
static cJSON *get_wifi_sta_info(void)
{
	tcpip_adapter_ip_info_t ip_info = {0};
	wifi_ap_record_t ap_info = {0};
	esp_err_t rc;

	cJSON *js = cJSON_CreateObject();
	if(!js)
		return NULL;

	rc = esp_wifi_sta_get_ap_info(&ap_info);
	cJSON_AddStringToObject(js,"status",(rc == ESP_OK) ? "connected":
		(rc == ESP_ERR_WIFI_CONN) ? "not initialized":
		(rc == ESP_ERR_WIFI_NOT_CONNECT) ? "disconnected":"??");
		
	if(rc == ESP_OK)
		cJSON_AddItemToObject(js,"connection",ap_record_to_json(&ap_info));

	if(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA,&ip_info) == ESP_OK)
		cJSON_AddItemToObject(js,"ip_info",ip_info_to_json(&ip_info));
	return js;
}

esp_err_t wifi_info_handler(httpd_req_t *req)
{
	cJSON *js = cJSON_CreateObject();
	if(!js)
		return ESP_FAIL;

	cJSON_AddItemToObject(js,"ap",get_wifi_ap_info());
	cJSON_AddItemToObject(js,"sta",get_wifi_sta_info());

	char *js_txt = cJSON_Print(js);
	cJSON_Delete(js);

	httpd_resp_set_type(req,HTTPD_TYPE_JSON);
	httpd_resp_send(req, js_txt, -1);
	free(js_txt);
	return ESP_OK;
}

esp_err_t wifi_scan_handler(httpd_req_t *req)
{
	uint16_t ap_num = DEFAULT_SCAN_LIST_SIZE;
	uint16_t ap_found = 0;

	wifi_ap_record_t ap_list[DEFAULT_SCAN_LIST_SIZE] = {0};

	ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_list));
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_found));

	ESP_LOGI(TAG, "Total APs scanned = %u", ap_found);

	cJSON* js_array = cJSON_CreateArray();
	if(!js_array)
		return ESP_FAIL;

	for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_found); i++) {
		ESP_LOGI(TAG, "\tSSID %s", ap_list[i].ssid);
		ESP_LOGI(TAG, "\tRSSI %d", ap_list[i].rssi);
		ESP_LOGI(TAG, "\tchannel %d\n", ap_list[i].primary);
		cJSON_AddItemToArray(js_array, ap_record_to_json(&ap_list[i]));
	}
	cJSON* js_obj = cJSON_CreateObject();
	if(!js_obj){
		cJSON_Delete(js_array);
		return ESP_FAIL;
	}
	cJSON_AddItemToObject(js_obj,"scan",js_array);

	char *js_txt = cJSON_Print(js_obj);
	cJSON_Delete(js_obj);
	if(!js_txt)
		return ESP_FAIL;

	httpd_resp_set_type(req,HTTPD_TYPE_JSON);
	httpd_resp_send(req, js_txt, -1);
	free(js_txt);
	return ESP_OK;
}

esp_err_t wifi_connect_handler(httpd_req_t *req)
{
	char *ssid = NULL;
	char *password = NULL;
	char *param;
	char buf[128]={0};

	int recv, bytes_left = req->content_len;
	wifi_config_t wifi_config = { 0 };
	wifi_mode_t wifi_mode;

	esp_wifi_get_mode(&wifi_mode);
	if(wifi_mode == WIFI_MODE_AP)
		return ESP_FAIL;

	// Read request data
	while(bytes_left > 0) {
		recv = httpd_req_recv(req, buf, MIN(bytes_left, sizeof(buf)));
		if(recv <= 0) {
			// Retry receiving if timeout occurred
			if (recv == HTTPD_SOCK_ERR_TIMEOUT)
				continue;
			return ESP_FAIL;
		}
		bytes_left -= recv;
	}
	/* in request content We expect query like: ssid=MY_SSID&passwd=MY_PASSWD
	Use strtok to divide query into tokens*/
	param=strtok(buf,"&");
	do {
		if(param == NULL) //empty parameter
			break;
		if( strncmp(param,"ssid=",5) == 0 )
			ssid = strchr(param,'=')+1;
		else if( strncmp(param,"passwd=",7) == 0 )
			password = strchr(param,'=')+1;
	} while( (param=strtok(NULL,"&")) != NULL);

	if(!ssid || !password)
		return ESP_FAIL;

	strncpy((char*)wifi_config.sta.ssid, ssid,32);
	strncpy((char*)wifi_config.sta.password, password,64);

	esp_wifi_disconnect();
	esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
	esp_wifi_connect();

	httpd_resp_set_status(req, "200 OK");
	httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
	httpd_resp_send(req, "", -1);
	return ESP_OK;
}
