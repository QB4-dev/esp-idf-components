#ifndef _ESP_WAV_OBJECT_H_
#define _ESP_WAV_OBJECT_H_

#include <stdint.h>
#include <stdbool.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_system.h>
#include <esp_err.h>
#include <spiffs_config.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	WAV_NONE   = 0,
	WAV_EMBED  = 1,
	WAV_SPIFFS = 2,
} wav_obj_type_t;

struct embed_wav_data{
	const uint8_t *addr;
};

struct spiffs_wav_data{
	char path[SPIFFS_OBJ_NAME_LEN];
};

typedef struct {
	wav_obj_type_t type;

	union {
		struct embed_wav_data  embed;
		struct spiffs_wav_data spiffs;
	}data;
}wav_obj_t;

typedef struct {
	wav_obj_type_t type;
	union {
		uint8_t *embed_rdptr; //embedded data read pointer
		int      spiffs_fd;   //SPIFFS file descriptor
	}io;
}wav_handle_t;

//Common .wav file properties
typedef struct {
	uint8_t  num_channels;
	uint8_t  bit_depth;
	uint32_t sample_rate;
	uint32_t sample_alignment;
	uint32_t data_bytes;
}wav_properties_t;

bool wav_object_open(wav_obj_t *wav, wav_handle_t *wavh);
int wav_object_read(wav_handle_t *wavh, void *buf, size_t count);
int wav_object_close(wav_handle_t *wavh);

#ifdef __cplusplus
}
#endif

/**@}*/

#endif  // _ESP_WAV_OBJECT_H_
