#include "include/timezone.h"

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <esp_log.h>

static const char *TAG = "TZ";

/**
 * @brief Array of timezone name / POSIX TZ string pairs
 *
 * The list is populated from `tz_data.txt` at compile time. Each entry is a
 * 2-element array where index 0 is the timezone name (e.g. "Europe/Warsaw") and
 * index 1 is the corresponding POSIX TZ string (e.g. "CET-1CEST,M3.5.0/2,M10.5.0/3").
 */
static const char *tz_data[][2] = {
#include "tz_data.txt"
    { NULL, NULL }
};

static const char *find_tz(const char *name)
{
    if (name != NULL) {
        int index = 0;
        while (true) {
            if (tz_data[index][0] == NULL) {
                return NULL;
            }
            if (strcmp(tz_data[index][0], name) == 0) {
                return tz_data[index][1];
            }
            index++;
        }
    }
    return NULL;
}


esp_err_t tzdata_set_timezone(char *tz_name)
{
    const char *tz = find_tz(tz_name);

    /* Set timezone https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv */
    if (tz) {
        ESP_LOGI(TAG, "Update timezone: %s : %s", tz_name, tz);
        /* Set TZ environment variable and apply */
        setenv("TZ", tz, 1);
        tzset();
    } else {
        ESP_LOGW(TAG, "Unknown timezone %s", tz_name);
    }
    return ESP_OK;
}

