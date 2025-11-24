/*
 * Copyright (c) 2025 <qb4.dev@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <esp_err.h>
#include <esp_http_server.h>

#include "settings-defs.h"

/**
 * @brief Type of callback used to handle or process a settings group.
 *
 * Handlers of this type can be registered with the settings subsystem
 * and will be invoked with a pointer to a settings group and an
 * arbitrary user-supplied argument.
 *
 * @param settings Pointer to the settings group to handle.
 * @param arg Optional user-provided argument passed through registration.
 * @return esp_err_t ESP_OK on success, otherwise an ESP error code.
 */
typedef esp_err_t (*settings_handler_t)(const settings_group_t *settings, void *arg);

/**
 * @brief Print a settings group to the console/log.
 *
 * This helper prints the contents of @p settings in a human-readable
 * form for debugging purposes.
 *
 * @param settings Pointer to the settings group to print. Must not be NULL.
 */
void settings_pack_print(const settings_group_t *settings);

/**
 * @brief Find a setting by group and identifier.
 *
 * Searches the provided settings pack for a setting that belongs to the
 * group named @p gr and has the identifier @p id.
 *
 * @param settings Pointer to the settings pack to search. Must not be NULL.
 * @param gr Null-terminated group name to search for.
 * @param id Null-terminated setting identifier to search for.
 * @return setting_t* Pointer to the matching setting, or NULL if not found.
 */
setting_t *settings_pack_find(const settings_group_t *settings, const char *gr, const char *id);

/**
 * @brief Initialize a single setting to its default value.
 *
 * Sets the provided @p setting to its defined default state. If the
 * setting holds dynamically allocated resources, the caller remains
 * responsible for their lifetime according to the setting's definition.
 *
 * @param setting Pointer to the setting to initialize. Must not be NULL.
 */
void setting_set_defaults(setting_t *setting);

/**
 * @brief Initialize all settings in a settings pack to their defaults.
 *
 * Iterates through the settings contained in @p settings_pack and calls
 * `setting_set_defaults()` for each entry.
 *
 * @param settings_pack Pointer to the settings pack to initialize. Must not be NULL.
 */
void settings_pack_set_defaults(const settings_group_t *settings_pack);

/**
 * @brief Read settings from NVS into the provided settings pack.
 *
 * Reads persisted values from NVS and populates the in-memory settings
 * pack. Existing in-memory values may be overwritten.
 *
 * @param settings Pointer to the settings pack to populate. Must not be NULL.
 * @return esp_err_t ESP_OK on success; otherwise an error code from esp_err.h.
 */
esp_err_t settings_nvs_read(const settings_group_t *settings);

/**
 * @brief Write the provided settings pack to NVS.
 *
 * Persists all relevant settings contained in @p settings to non-volatile
 * storage so they survive reboots.
 *
 * @param settings Pointer to the settings pack to persist. Must not be NULL.
 * @return esp_err_t ESP_OK on success; otherwise an error code from esp_err.h.
 */
esp_err_t settings_nvs_write(const settings_group_t *settings);

/**
 * @brief Erase all settings stored in NVS.
 *
 * Removes the settings namespace from NVS. Use with care — this is not
 * reversible and will remove persisted configuration.
 * 
 * @param settings Pointer to the settings pack to populate. Must not be NULL.
 * @return esp_err_t ESP_OK on success; otherwise an error code from esp_err.h.
 */
esp_err_t settings_nvs_erase(settings_group_t *settings);

/**
 * @brief Register a settings handler callback.
 *
 * The provided @p handler will be stored and invoked according to the
 * settings subsystem's semantics (e.g. on apply, on load). The @p arg is
 * forwarded to the handler when it is invoked.
 *
 * @param handler Function pointer to the handler to register. Must not be NULL.
 * @param arg User-defined argument passed to the handler when invoked.
 * @return esp_err_t ESP_OK on success; otherwise an error code.
 */
esp_err_t settings_handler_register(settings_handler_t handler, void *arg);

/**
 * @brief HTTP server handler for serving or updating settings.
 *
 * This function is intended to be used as an ESP HTTPD request handler
 * and implements the settings HTTP endpoint.
 *
 * @param req Pointer to the HTTP request provided by the ESP HTTP server.
 * @return esp_err_t ESP_OK if the request was handled successfully; otherwise an error code.
 */
esp_err_t settings_httpd_handler(httpd_req_t *req);

#endif /* SETTINGS_H_ */
