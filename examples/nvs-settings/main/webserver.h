#ifndef WEBSERVER_H_
#define WEBSERVER_H_

#include <esp_err.h>
#include <settings.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t webserver_init(const settings_group_t device_settings[]);
void      webserver_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // WEBSERVER_H_