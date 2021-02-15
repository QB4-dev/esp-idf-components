#ifndef _ESP_WAV_PLAYER_H_
#define _ESP_WAV_PLAYER_H_

#include <stdint.h>
#include <stdbool.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include <esp_system.h>
#include <esp_err.h>
#include <driver/gpio.h>
#include <driver/i2s.h>

#include <esp_wav_object.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	SemaphoreHandle_t mutex;
	i2s_port_t i2s_port;
	uint8_t task_priority;
	uint8_t volume;
	bool is_playing;
	bool tda1543_mode;  //for TDA1543
	uint8_t queue_len;
	QueueHandle_t queue;
	EventGroupHandle_t event_group;

	bool has_amp_pwr_ctl;
	gpio_num_t amp_power_gpio;
	int amp_power_on_delay;
	int amp_power_off_delay;
}esp_wav_player_t;

/* wav player events */
#define ESP_WAV_PLAYER_STARTED BIT0
#define ESP_WAV_PLAYER_STOPPED BIT1

esp_err_t esp_wav_player_init(esp_wav_player_t *player, i2s_pin_config_t *i2s_pin_conf, i2s_config_t *i2s_conf);
esp_err_t esp_wav_player_deinit(esp_wav_player_t *player);

esp_err_t esp_wav_player_get_play_state(esp_wav_player_t *player, bool *state);

esp_err_t esp_wav_player_set_volume(esp_wav_player_t *player, uint8_t vol);
esp_err_t esp_wav_player_get_volume(esp_wav_player_t *player, uint8_t *vol);

esp_err_t esp_wav_player_play(esp_wav_player_t *player, wav_obj_t *wav);
esp_err_t esp_wav_player_stop(esp_wav_player_t *player);

esp_err_t esp_wav_player_add_to_queue(esp_wav_player_t *player, wav_obj_t *wav);
esp_err_t esp_wav_player_get_queued(esp_wav_player_t *player, uint8_t *queued);
esp_err_t esp_wav_player_reset_queue(esp_wav_player_t *player);
esp_err_t esp_wav_player_play_queued(esp_wav_player_t *player);

#ifdef __cplusplus
}
#endif

/**@}*/

#endif  // _ESP_WAV_PLAYER_H_
