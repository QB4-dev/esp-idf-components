#include "webserver.h"

#include <string.h>
#include <inttypes.h>

#include <esp_log.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_http_server.h>

#ifndef CONFIG_IDF_TARGET_ESP8266
#include <esp_mac.h>
#endif

#if CONFIG_IDF_TARGET_ESP8266
#define ESP_WIFI_AP_SSID "ESP8266-AP"
#else
#define ESP_WIFI_AP_SSID "ESP32-AP"
#endif

#if CONFIG_IDF_TARGET_ESP8266

#define DECLARE_EMBED_HANDLER(NAME, URI, CT)                             \
    extern const char embed_##NAME[] asm("_binary_" #NAME "_start");     \
    extern const char size_##NAME[] asm("_binary_" #NAME "_size");       \
    esp_err_t         get_##NAME(httpd_req_t *req)                       \
    {                                                                    \
        httpd_resp_set_type(req, CT);                                    \
        return httpd_resp_send(req, embed_##NAME, (size_t)&size_##NAME); \
    }                                                                    \
    static const httpd_uri_t route_get_##NAME = { .uri = (URI), .method = HTTP_GET, .handler = get_##NAME }

#else

#define DECLARE_EMBED_HANDLER(NAME, URI, CT)                           \
    extern const char   embed_##NAME[] asm("_binary_" #NAME "_start"); \
    extern const size_t size_##NAME asm(#NAME "_length");              \
    esp_err_t           get_##NAME(httpd_req_t *req)                   \
    {                                                                  \
        httpd_resp_set_type(req, CT);                                  \
        return httpd_resp_send(req, embed_##NAME, size_##NAME);        \
    }                                                                  \
    static const httpd_uri_t route_get_##NAME = { .uri = (URI), .method = HTTP_GET, .handler = get_##NAME }
#endif

static const char    *TAG = "WEBSRV";
static httpd_handle_t server = NULL;

/* Embedded file handlers (same as before) */
DECLARE_EMBED_HANDLER(index_html, "/index.html", "text/html");
DECLARE_EMBED_HANDLER(style_css, "/style.css", "text/css");
DECLARE_EMBED_HANDLER(script_js, "/script.js", "text/javascript");

static const httpd_uri_t route_get_root = { .uri = "/", .method = HTTP_GET, .handler = get_index_html };

static httpd_uri_t settings_get_handler = { .uri = "/settings", .method = HTTP_GET, .handler = settings_httpd_handler };

static httpd_uri_t settings_post_handler = { .uri = "/settings",
                                             .method = HTTP_POST,
                                             .handler = settings_httpd_handler };

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_AP_STACONNECTED: {
            wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
            ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
        } break;
        case WIFI_EVENT_AP_STADISCONNECTED: {
            wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
            ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
        } break;
        case WIFI_EVENT_STA_START:
            break;
        case WIFI_EVENT_STA_CONNECTED: {
            wifi_event_sta_connected_t *info = event_data;
            ESP_LOGI(TAG, "Connected to SSID: %s", info->ssid);
        } break;
        case WIFI_EVENT_STA_DISCONNECTED: {
            wifi_event_sta_disconnected_t *info = event_data;
            ESP_LOGE(TAG, "Station disconnected(reason : %d)", info->reason);
            if (info->reason == WIFI_REASON_ASSOC_LEAVE)
                break;
            esp_wifi_connect();
        } break;
        default:
            break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
        case IP_EVENT_STA_GOT_IP: {
            ip_event_got_ip_t *event = event_data;
            ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        } break;
        default:
            break;
        }
    }
}

static esp_err_t wifi_init_softap(void)
{
    wifi_config_t wifi_config = { .ap = {
                                      .ssid = ESP_WIFI_AP_SSID,
                                      .ssid_len = strlen(ESP_WIFI_AP_SSID),
                                      .max_connection = 1,
                                      .authmode = WIFI_AUTH_OPEN,
                                  } };

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

#ifndef CONFIG_IDF_TARGET_ESP8266
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();
#endif

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s", ESP_WIFI_AP_SSID);
    return ESP_OK;
}

esp_err_t webserver_init(const settings_group_t device_settings[])
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Initializing WiFi and HTTP server");

    /* init WiFi (AP+STA) */
    ESP_ERROR_CHECK(wifi_init_softap());

    /* start HTTP server */
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    /* register static embedded files */
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &route_get_root));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &route_get_index_html));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &route_get_script_js));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &route_get_style_css));

    /* register CGI-like handlers for settings */
    settings_get_handler.user_ctx = (void *)device_settings;
    settings_post_handler.user_ctx = (void *)device_settings;

    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &settings_get_handler));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &settings_post_handler));

    ESP_LOGI(TAG, "server started on port %d, free mem: %" PRIu32 " bytes", config.server_port,
             esp_get_free_heap_size());
    return ESP_OK;
}

void webserver_deinit(void)
{
    if (server) {
        ESP_LOGI(TAG, "Stopping HTTP server");
        httpd_stop(server);
        server = NULL;
    }
    /* unregister event handlers (ignore errors) */
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler);
    esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler);
}