/*
 * Copyright (c) 2025 <qb4.dev@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_event.h>

#include "webserver.h"

static const char *TAG = "APP";

/* single place for group IDs (only IDs required) */
#define GROUP_DEVICE_ID "DEV"
#define GROUP_TIMER_ID "TMR"
#define GROUP_SAFETY_ID "SAFE"
#define GROUP_NETWORK_ID "NET"

/* helper buffers / option lists */
static char        hostname[32];
static char        tz_buf[32] = "UTC";
static const char *wifi_mode_opts[] = { "STA", "AP", "APSTA", NULL };

/* DEVICE settings (basic + colors) */
static setting_t device_settings_items[] = {
    { .id = "ENABLED", .label = "Device enabled", .type = SETTING_TYPE_BOOL, .boolean = { .val = true, .def = true } },

    { .id = "NAME",
      .label = "Device name",
      .type = SETTING_TYPE_TEXT,
      .text = { .val = hostname, .def = "def-hostname", .len = sizeof(hostname) } },

    { .id = "DISPBR",
      .label = "Display brightness",
      .type = SETTING_TYPE_NUM,
      .num = { .val = 5, .def = 5, .range = { 1, 7 } } },

    /* Color settings for UI/LED */
    { .id = "LEDCLR",
      .label = "LED color",
      .type = SETTING_TYPE_COLOR,
      .color = { .combined = 0x00FF00 } }, /* default green */

    { .id = "UICLR",
      .label = "UI color",
      .type = SETTING_TYPE_COLOR,
      .color = { .combined = 0xFFFFFF } }, /* default white */

    {} /* terminator */
};

/* TIMER settings */
static setting_t timer_settings_items[] = {
    { .id = "PRESET1",
      .label = "Preset 1 (min)",
      .type = SETTING_TYPE_NUM,
      .num = { .val = 15, .def = 15, .range = { 0, 240 } } },

    { .id = "PRESET2",
      .label = "Preset 2 (min)",
      .type = SETTING_TYPE_NUM,
      .num = { .val = 30, .def = 30, .range = { 0, 240 } } },

    { .id = "PRESET3",
      .label = "Preset 3 (min)",
      .type = SETTING_TYPE_NUM,
      .num = { .val = 60, .def = 60, .range = { 0, 240 } } },

    { .id = "AUTO_START",
      .label = "Auto-start on power",
      .type = SETTING_TYPE_BOOL,
      .boolean = { .val = false, .def = false } },

    {} /* terminator */
};

/* SAFETY settings */
static setting_t safety_settings_items[] = {
    { .id = "VOLT_TH",
      .label = "Voltage threshold (V)",
      .type = SETTING_TYPE_NUM,
      .num = { .val = 200, .def = 200, .range = { 0, 400 } } },

    { .id = "RELAY_ACT",
      .label = "Relay active high",
      .type = SETTING_TYPE_BOOL,
      .boolean = { .val = true, .def = true } },

    { .id = "PWR_FAIL",
      .label = "On power loss",
      .type = SETTING_TYPE_ONEOF,
      .oneof = { .val = 0, .def = 0, .options = (const char *[]){ "STOP", "PAUSE", "IGNORE", NULL } } },

    {} /* terminator */
};

/* NETWORK settings (includes timezone + date/time settings) */
static setting_t network_settings_items[] = {
    /* timezone setting */
    { .id = "TZ",
      .label = "Timezone",
      .type = SETTING_TYPE_TIMEZONE,
      .timezone = { .val = tz_buf, .def = "UTC", .len = sizeof(tz_buf) } },
    { .id = "WIFI_MODE",
      .label = "Wi‑Fi mode",
      .type = SETTING_TYPE_ONEOF,
      .oneof = { .val = 2, .def = 2, .options = wifi_mode_opts } },

/*
     * Date / Time settings - useful for manual time tuning or schedule testing.
     * These fields are optional depending on compile-time support in settings-defs.
     */
#ifdef CONFIG_SETTINGS_DATETIME_SUPPORT
    { .id = "DATE", .label = "Local date", .type = SETTING_TYPE_DATE, .date = { .day = 1, .month = 1, .year = 2025 } },

    { .id = "TIME", .label = "Local time", .type = SETTING_TYPE_TIME, .time = { .hh = 12, .mm = 0 } },

    { .id = "SYS",
      .label = "System date/time",
      .type = SETTING_TYPE_DATETIME,
      .datetime = { .date = { .day = 1, .month = 1, .year = 2025 }, .time = { .hh = 8, .mm = 0 } } },
#endif

    {} /* terminator */
};

/* top-level settings groups */
static const settings_group_t app_settings[] = {
    { .id = GROUP_DEVICE_ID, .label = "Device", .settings = device_settings_items },
    { .id = GROUP_TIMER_ID, .label = "Timer", .settings = timer_settings_items },
    { .id = GROUP_SAFETY_ID, .label = "Safety", .settings = safety_settings_items },
    { .id = GROUP_NETWORK_ID, .label = "Network", .settings = network_settings_items },
    {} /* terminator */
};

static esp_err_t on_settings_changed(const settings_group_t *settings, void *arg)
{
    ESP_LOGI(TAG, "settings changed");
    settings_pack_print(app_settings);

    /* example usage of setting */
    setting_t *dispbr_setting = settings_pack_find(settings, GROUP_DEVICE_ID, "DISPBR");
    uint8_t    brightness = dispbr_setting ? dispbr_setting->num.val : 7;
    ESP_LOGW(TAG, "-> set display brightness to %d", brightness);

    /* example: read LED color if available */
#ifdef CONFIG_SETTINGS_COLOR_SUPPORT
    setting_t *ledclr = settings_pack_find(settings, GROUP_DEVICE_ID, "LEDCLR");
    if (ledclr) {
        ESP_LOGW(TAG, "-> LED color: 0x%06" PRIx32, ledclr->color.combined);
    }
#endif

    return ESP_OK;
}

void app_main(void)
{
    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    settings_nvs_read(app_settings);
    settings_pack_print(app_settings);
    settings_handler_register(on_settings_changed, NULL);

    ESP_LOGI(TAG, "Starting webserver + WiFi (APSTA)");
    ESP_ERROR_CHECK(webserver_init(app_settings));
}
