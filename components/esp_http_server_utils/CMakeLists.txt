register_component()

idf_component_register(
	REQUIRES "http_parser tcpip_adapter esp_http_server app_update json spiffs"
	SRC_DIRS "."
	INCLUDE_DIRS "."
)