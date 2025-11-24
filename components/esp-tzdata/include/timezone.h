#ifndef TIMEZONE_H_
#define TIMEZONE_H_

#include <esp_err.h>

/**
 * @file timezone.h
 * @brief Timezone helper API
 *
 * @defgroup timezone Timezone
 * @brief Utilities for working with timezone data (tzdata)
 *
 * Functions in this module allow selecting a timezone by name from the
 * compiled tz database and applying it to the system runtime.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set the current timezone by name
 *
 * Sets the device timezone using the compiled tzdata. The timezone name
 * should match an entry from the tz database (for example: "Europe/Warsaw",
 * "UTC", "America/New_York"). The string must be null-terminated.
 *
 * @param tz_name Pointer to a null-terminated timezone name string.
 *
 * @return
 *      - `ESP_OK` on success
 *      - `ESP_ERR_INVALID_ARG` if `tz_name` is NULL or invalid
 *      - `ESP_FAIL` on other failures
 */
esp_err_t tzdata_set_timezone(char *tz_name);

#ifdef __cplusplus
}
#endif

#endif /* TIMEZONE_H */
