#ifndef ESP_WAV_PLAYER_WAV_HANDLE_H_
#define ESP_WAV_PLAYER_WAV_HANDLE_H_

#include <stdint.h>
#include <stddef.h>

#include "include/wav_object.h"

typedef struct wav_handle wav_handle_t;

struct wav_handle {
    void *ctx;                                              /*!< Backend-specific context pointer. */
    int (*open)(wav_handle_t *h);                           /*!< Open/initialize the backend (returns 0 on success). */
    size_t (*read)(wav_handle_t *h, void *buf, size_t len); /*!< Read up to `len` bytes into `buf`. */
    int (*seek)(wav_handle_t *h, size_t offset);            /*!< Seek to `offset` within the WAV data. */
    void (*close)(wav_handle_t *h);                         /*!< Close the backend and release any resources. */
    void (*clean_ctx)(wav_handle_t *h);                     /*!< Optional cleanup function for `ctx`. */

    /* Filled by wav_parse_header() */
    uint16_t num_channels;     /*!< Number of audio channels. */
    uint32_t sample_rate;      /*!< Sampling rate in Hz. */
    uint32_t byte_rate;        /*!< Bytes per second (sample_rate * block_align). */
    uint32_t sample_alignment; /*!< Block align: number of bytes per sample frame. */
    uint16_t bit_depth;        /*!< Bits per sample (e.g. 16). */
    size_t   data_start;       /*!< Offset (in bytes) from start of file to audio data. */
    size_t   data_bytes;       /*!< Number of bytes in the audio data chunk. */
};

wav_handle_t *wav_backend_embed_create(const uint8_t *start);
wav_handle_t *wav_backend_file_create(const char *path);

// backend-independent creator
wav_handle_t *wav_handle_init(const wav_obj_t *src);
void          wav_handle_free(wav_handle_t *h);

// parses header and fills h->size, h->sample_rate, etc.
int wav_parse_header(wav_handle_t *h);

#endif /* ESP_WAV_PLAYER_WAV_HANDLE_H_ */
