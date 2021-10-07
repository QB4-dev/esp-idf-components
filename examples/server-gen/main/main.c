/* Simple WAV playback example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_system.h>
#include <esp_log.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <nvs_flash.h>
#include <esp_event_loop.h>
#include <tcpip_adapter.h>
#include <mdns.h>


#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include "server.h"

static const char *TAG="APP";

static httpd_handle_t server = NULL;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;
/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	/* For accessing reason codes in case of disconnection */
	system_event_info_t *info = &event->event_info;

	switch(event->event_id) {
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		ESP_LOGI(TAG, "got ip:%s",ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
		xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_AP_STACONNECTED:
		ESP_LOGI(TAG, "station:"MACSTR" join, AID=%d",
				 MAC2STR(event->event_info.sta_connected.mac),
				 event->event_info.sta_connected.aid);
		break;
	case SYSTEM_EVENT_AP_STADISCONNECTED:
		ESP_LOGI(TAG, "station:"MACSTR"leave, AID=%d",
				 MAC2STR(event->event_info.sta_disconnected.mac),
				 event->event_info.sta_disconnected.aid);
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		ESP_LOGE(TAG, "Disconnect reason : %d", info->disconnected.reason);
		if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
			/*Switch to 802.11 bgn mode */
			esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
		}
		esp_wifi_connect();
		xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_AP_STAIPASSIGNED:
		ESP_LOGI(TAG, "assigned ip:%s", ip4addr_ntoa(&event->event_info.ap_staipassigned.ip));
		break;
	default:
		break;
	}
	mdns_handle_system_event(ctx, event);
	return ESP_OK;
}

void wifi_init_softap(void)
{
    wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	wifi_config_t wifi_config_ap = {
		.ap = {
			.ssid = "esp8266",
			.ssid_len = strlen("esp8266"),
			.password = "",
			.max_connection = 4,
			.authmode = WIFI_AUTH_OPEN
		},
	};

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config_ap));
    ESP_ERROR_CHECK(esp_wifi_start());
}


void app_main()
{
	ESP_LOGI(TAG, "server gen demo");
	
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	
	ESP_ERROR_CHECK(mdns_init());
	ESP_ERROR_CHECK(mdns_hostname_set("esp8266"));
	
	wifi_init_softap();
	server = start_webserver();
	for (;;) {
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}
