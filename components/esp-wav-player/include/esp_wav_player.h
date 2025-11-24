#ifndef _ESP_WAV_PLAYER_H_
#define _ESP_WAV_PLAYER_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2s.h"
#include "wav_object.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file esp_wav_player.h
 * @brief Simple WAV player for ESP-IDF (I2S output).
 */

/**
 * @brief Opaque handle for a WAV player instance.
 *
 * Users should treat this as an opaque pointer returned by
 * `esp_wav_player_init` and passed to other API functions.
 */
typedef void *esp_wav_player_t;

/**
 * @brief Callback invoked for WAV player events (start/end).
 *
 * @param wav_player Handle to the WAV player instance that generated the event.
 * @param arg User-provided argument (set when registering the callback).
 */
typedef void (*esp_wav_player_cb_t)(esp_wav_player_t wav_player, void *arg);

/**
 * @brief Playback state for the WAV player.
 */
typedef enum {
    ESP_WAV_PLAYER_STOPPED, /*!< Playback stopped or not started. */
    ESP_WAV_PLAYER_PLAYING, /*!< WAV is actively playing. */
    ESP_WAV_PLAYER_PAUSED   /*!< Playback is paused. */
} esp_wav_player_state_t;

/**
 * @brief Configuration structure used to initialize a WAV player instance.
 *
 * @note The `i2s_pin_config` and `base_cfg` fields allow full control of the
 * I2S peripheral behaviour; defaults are provided by `ESP_WAV_PLAYER_DEFAULT_CONFIG()`.
 */
typedef struct {
    int              i2s_num;        /*!< I2S peripheral number (e.g. `I2S_NUM_0`). */
    i2s_pin_config_t i2s_pin_config; /*!< I2S pin mapping used to route signals to GPIOs. */
    i2s_config_t     base_cfg;       /*!< Base I2S runtime configuration (sample rate, format, buffers). */
    size_t           queue_len;      /*!< Queue length for internal command/notification queue. */
} esp_wav_player_config_t;

#if CONFIG_IDF_TARGET_ESP8266
/**
 * @brief Default configuration for ESP8266 targets.
 *
 * Use as `esp_wav_player_config_t cfg = ESP_WAV_PLAYER_DEFAULT_CONFIG();`.
 */
#define ESP_WAV_PLAYER_DEFAULT_CONFIG() \
    {                                                                          \
    .i2s_num = I2S_NUM_0,                                                      \
    .i2s_pin_config = {                                                        \
        .bck_o_en = 1,                                                         \
        .ws_o_en = 1,                                                          \
        .data_out_en = 1,                                                      \
    },                                                                         \
    .base_cfg = {                                                              \
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,                                 \
        .sample_rate = 22050,                                                  \
        .bits_per_sample = 16,                                                 \
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                          \
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_LSB, \
        .dma_buf_count = 4,                                                    \
        .dma_buf_len = 256,                                                    \
        .tx_desc_auto_clear = true                                             \
    },                                                                         \
    .queue_len = 4                                                             \
}
#else
/**
 * @brief Default configuration for non-ESP8266 targets (ESP32 etc.).
 *
 * The example mapping uses `.bck_io_num = GPIO_NUM_32`, `.ws_io_num = GPIO_NUM_25`
 * and `.data_out_num = GPIO_NUM_33`. Adjust these GPIOs in your board-specific
 * setup as required.
 */
#define ESP_WAV_PLAYER_DEFAULT_CONFIG() \
    {                                                      \
    .i2s_num = I2S_NUM_0,                                  \
    .i2s_pin_config = {                                    \
        .bck_io_num = GPIO_NUM_32,                         \
        .ws_io_num = GPIO_NUM_25,                          \
        .data_out_num = GPIO_NUM_33,                       \
    },                                                     \
    .base_cfg = {                                          \
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,             \
        .sample_rate = 22050,                              \
        .bits_per_sample = 16,                             \
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,      \
        .communication_format = I2S_COMM_FORMAT_STAND_MSB, \
        .dma_buf_count = 4,                                \
        .dma_buf_len = 256,                                \
        .tx_desc_auto_clear = true                         \
    },                                                     \
    .queue_len = 4                                         \
}
#endif

/**
 * @brief Initialize a WAV player instance.
 *
 * @param[out] player Pointer that will receive the allocated player handle on success.
 * @param[in] config Pointer to configuration; if NULL, behaviour is undefined —
 *                   callers should pass a valid config (you can use
 *                   `ESP_WAV_PLAYER_DEFAULT_CONFIG()` to get sensible defaults).
 * @return ESP_OK on success, otherwise an `esp_err_t` error code.
 */
esp_err_t esp_wav_player_init(esp_wav_player_t *player, const esp_wav_player_config_t *config);

/**
 * @brief Deinitialize and free a WAV player instance.
 *
 * @param hdl Handle previously returned by `esp_wav_player_init`.
 * @return ESP_OK on success, otherwise an `esp_err_t` error code.
 */
esp_err_t esp_wav_player_deinit(esp_wav_player_t hdl);

/**
 * @brief Start playback of a WAV source.
 *
 * This function may queue the WAV for playback depending on implementation and
 * the configured queue length.
 *
 * @param player Initialized player handle.
 * @param src Pointer to a `wav_obj_t` describing the WAV data to play.
 * @return ESP_OK on success, otherwise an `esp_err_t` error code.
 */
esp_err_t esp_wav_player_play(esp_wav_player_t player, const wav_obj_t *src);

/**
 * @brief Stop playback immediately.
 *
 * @param player Player handle.
 * @return ESP_OK on success, otherwise an `esp_err_t` error code.
 */
esp_err_t esp_wav_player_stop(esp_wav_player_t player);

/**
 * @brief Pause playback.
 *
 * @param player Player handle.
 * @return ESP_OK on success, otherwise an `esp_err_t` error code.
 */
esp_err_t esp_wav_player_pause(esp_wav_player_t player);

/**
 * @brief Get the current player state.
 *
 * @param player Player handle.
 * @param[out] st Pointer to receive the state value.
 * @return ESP_OK on success, otherwise an `esp_err_t` error code.
 */
esp_err_t esp_wav_player_get_state(esp_wav_player_t player, esp_wav_player_state_t *st);

/**
 * @brief Set playback volume.
 *
 * @param player Player handle.
 * @param v Volume level (implementation-specific range, typically 0-255 or 0-100).
 * @return ESP_OK on success, otherwise an `esp_err_t` error code.
 */
esp_err_t esp_wav_player_set_volume(esp_wav_player_t player, uint8_t v);

/**
 * @brief Get current volume.
 *
 * @param hdl Player handle.
 * @param[out] vol Pointer to receive the current volume.
 * @return ESP_OK on success, otherwise an `esp_err_t` error code.
 */
esp_err_t esp_wav_player_get_volume(esp_wav_player_t hdl, uint8_t *vol);

/**
 * @brief Get number of queued items in the player's internal queue.
 *
 * @param hdl Player handle.
 * @param[out] qlen Pointer to receive queued count.
 * @return ESP_OK on success, otherwise an `esp_err_t` error code.
 */
esp_err_t esp_wav_player_get_queued(esp_wav_player_t hdl, size_t *qlen);

/**
 * @brief Register a callback invoked when playback starts.
 *
 * @param player Player handle.
 * @param cb Callback function or NULL to clear.
 * @param arg User argument passed to the callback.
 */
void esp_wav_player_set_start_cb(esp_wav_player_t player, esp_wav_player_cb_t cb, void *arg);

/**
 * @brief Register a callback invoked when playback ends.
 *
 * @param player Player handle.
 * @param cb Callback function or NULL to clear.
 * @param arg User argument passed to the callback.
 */
void esp_wav_player_set_end_cb(esp_wav_player_t player, esp_wav_player_cb_t cb, void *arg);

#ifdef __cplusplus
}
#endif

#endif // _ESP_WAV_PLAYER_H_