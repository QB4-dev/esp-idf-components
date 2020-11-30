#ifndef _ESP_WAV_PLAYER_H_
#define _ESP_WAV_PLAYER_H_

#include <stdint.h>
#include <stdbool.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_system.h>
#include <esp_err.h>
#include <driver/i2s.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	WAV_PROGMEM  = 0,
	WAV_SPIFFS   = 1,
} wav_obj_type_t;

typedef struct {
	uint8_t *file;
	uint8_t *read_ptr;
}embed_wav_obj_t;

typedef struct {
	const char *file;
	int fd;
}spiffs_wav_obj_t;

typedef struct {
	uint8_t  num_channels;
	uint8_t  bit_depth;
	uint32_t sample_rate;
	uint32_t sample_alignment;
	uint32_t data_bytes;
}wav_properties_t;

typedef struct {
	wav_obj_type_t type;

	union {
		embed_wav_obj_t  embed;
		spiffs_wav_obj_t spiffs;
	}data;

	//Common .wav file properties
	wav_properties_t props;
}wav_obj_t;

typedef struct {
	SemaphoreHandle_t mutex;
	i2s_port_t i2s_port;
	i2s_config_t i2s_config;
	i2s_pin_config_t i2s_pin_config;
	uint8_t volume;
	wav_obj_t *wav;
	bool is_playing;
	bool tda_1543_mode;  //for TDA1543
	uint8_t play_task_priority;
}esp_wav_player_t;

esp_err_t esp_wav_player_init(esp_wav_player_t *player);
esp_err_t esp_wav_player_deinit(esp_wav_player_t *player);

esp_err_t esp_wav_player_is_playing(esp_wav_player_t *player, bool *state);
esp_err_t esp_wav_player_set_volume(esp_wav_player_t *player, uint8_t vol);

esp_err_t esp_wav_player_play(esp_wav_player_t *player, wav_obj_t *wav);
esp_err_t esp_wav_player_stop(esp_wav_player_t *player);

#ifdef __cplusplus
}
#endif

/**@}*/

#endif  // _ESP_WAV_PLAYER_H_
