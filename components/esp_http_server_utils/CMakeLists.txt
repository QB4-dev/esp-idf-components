set(COMPONENT_SRCS "esp_http_server_utils.c esp_http_server_fota.c")
set(COMPONENT_ADD_INCLUDEDIRS ".")
set(COMPONENT_REQUIRES http_parser tcpip_adapter esp_http_server app_update json spiffs)

register_component()