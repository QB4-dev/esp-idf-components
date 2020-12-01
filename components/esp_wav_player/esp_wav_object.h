#ifndef _ESP_WAV_OBJECT_H_
#define _ESP_WAV_OBJECT_H_

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
	const uint8_t *file;
	const uint8_t *read_ptr;
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

bool wav_object_open(wav_obj_t *wav);
int wav_object_read(wav_obj_t *wav, void *buf, size_t count);
int wav_object_close(wav_obj_t *wav);

#ifdef __cplusplus
}
#endif

/**@}*/

#endif  // _ESP_WAV_OBJECT_H_
