# ESP HTTPD content generator demo

1. Add your files to __srv__ folder

1. Add server gen header `#include <esp_http_server_gen.h>`

1. Register generated handlers:
```
for(int i=0; i < generated_handlers_num; i++){
	ESP_ERROR_CHECK(httpd_register_uri_handler(server, &generated_handlers[i]));
	ESP_LOGI(TAG, "registered new URI: %s", generated_handlers[i].uri);
}
```
