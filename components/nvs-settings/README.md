# Component: Settings

Settings component for ESP-IDF providing a small, structured settings
subsystem with support for typed settings, default values, and
persistence to NVS. It includes helpers for printing and HTTP-based
exposure of settings via the ESP HTTP server.

**Features**
- Typed settings: boolean, integer, one-of (options), text, color, and
	optional date/time and timezone types (configured by Kconfig).
- Easy to add and manage new settings. Just define them once
- Read/write/erase persistence using ESP NVS.
- Simple HTTP handler integration to expose/update settings over HTTP.

## Quick start

- Add this component to your ESP-IDF project (as a component directory).
- Enable any optional features via your project's `sdkconfig` (e.g.
	`CONFIG_SETTINGS_DATETIME_SUPPORT`).
- Include headers in your code:

```c
#include "settings-defs.h"
#include "settings.h"
```

## Basic usage
Use static definitions of your device settings using data types from `settings-defs.h`

### Defining settings groups names
Settings are grouped in settings groups, it is good to use #define preprocessor directive to keep IDs in one place:
```c
#define GROUP_DEVICE_ID "DEV"
#define GROUP_TIMER_ID "TMR"
#define GROUP_SAFETY_ID "SAFE"
#define GROUP_NETWORK_ID "NET"
```
> [!NOTE]  
> Keep id's short because every setting will have crated own key for nvs storage by following pattern:
> `GROUP_ID`:`SETTING_ID` - the total length of `nvs_key` cannot be longer than `NVS_KEY_NAME_MAX_SIZE - 1` which is 16 characters with zero terminator at the end.
> If `nvs_key` will be too long the errors 	will be printed on debug output

### Defining actual settings

Define an arrays of settings for each group like:
```c
static char hostname[32]; //buffer for name setting

static setting_t device_settings_items[] = {
    {
        .id = "ENABLED",
        .label = "Device enabled",
        .type = SETTING_TYPE_BOOL,
        .boolean = { .val = true, .def = true }
    },
    { .id = "NAME",
      .label = "Device name",
      .type = SETTING_TYPE_TEXT,
      .text = { .val = hostname, .def = "def-hostname", .len = sizeof(hostname) } },

    { .id = "DISPBR",
      .label = "Display brightness",
      .type = SETTING_TYPE_NUM,
      .num = { .val = 5, .def = 5, .range = { 1, 7 } }
	},
    { .id = "LEDCOL",
      .label = "LED color",
      .type = SETTING_TYPE_COLOR,
      .color = { .combined = 0x00FF00 } }, /* default green */
    {} /* terminator */
};
```

### Defining global settings object

Create an array of defined settings groups like:
```c
/* top-level settings groups */
static const settings_group_t app_settings[] = {
    { .id = GROUP_DEVICE_ID, .label = "Device", .settings = device_settings_items },
    { .id = GROUP_TIMER_ID, .label = "Timer", .settings = timer_settings_items },
    { .id = GROUP_SAFETY_ID, .label = "Safety", .settings = safety_settings_items },
    { .id = GROUP_NETWORK_ID, .label = "Network", .settings = network_settings_items },
    {} /* terminator */
};
```
From this point all your settings can be accessed using `app_settings` variable

You can:

- Access your settings anywhere in your program:
  
```c
setting_t *dispbr_setting = settings_pack_find(app_settings, GROUP_DEVICE_ID, "DISPBR");
uint8_t    brightness = dispbr_setting ? dispbr_setting->num.val : 7; // 7 is fallback value if setting not defined
ESP_LOGW(TAG, "-> set display brightness to %d", brightness);
```

- Set defaults on whole settings pack:

```c
settings_pack_set_defaults(app_settings);
```

- Read persisted values from NVS (overwrites in-memory values):

```c
settings_nvs_read(app_settings);
```

- Store current values to NVS:

```c
settings_nvs_write(app_settings);
```

- Erase persisted settings (use with care):

```c
settings_nvs_erase(app_settings);
```

- Register a handler to be called by the settings subsystem(for example when settings are stored or erased):

```c
void my_handler(const settings_group_t *grp, void *arg) { /* ... */ }
settings_handler_register(my_handler, NULL);
```

- Serve settings over HTTP by registering `settings_httpd_handler` with the ESP HTTP server (see ESP HTTPD docs for handler registration).
  After registration of httpd handler settings will be available as json object in web browser - see an example project

**Configuration**

Optional features are controlled by Kconfig options (configured in
`sdkconfig`). Examples:

- `CONFIG_SETTINGS_DATETIME_SUPPORT` — enable time/date/datetime types
- `CONFIG_SETTINGS_TIMEZONE_SUPPORT` — enable timezone text type
- `CONFIG_SETTINGS_COLOR_SUPPORT` — enable color type

## Installation

### Using ESP Component Registry

[![Component Registry](https://components.espressif.com/components/qb4-dev/settings/badge.svg)](https://components.espressif.com/components/qb4-dev/settings)

```bash
idf.py add-dependency "qb4-dev/settings=*"
```

### Manual Installation

Clone this repository into your project's `components` directory:

```bash
cd your_project/components
git clone https://github.com/QB4-dev/esp-settings.git esp-settings
```

