#include "wav_handle.h"
#include "wav_header.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <esp_log.h>

static const char *TAG = "WAVH";

wav_handle_t *wav_handle_init(const wav_obj_t *src)
{
    if (!src)
        return NULL;

    switch (src->type) {
    case WAV_SRC_EMBED:
        return wav_backend_embed_create(src->embed.addr);

    case WAV_SRC_SPIFFS:
    case WAV_SRC_MMC:
        return wav_backend_file_create(src->spiffs.path);

    default:
        return NULL;
    }
}

void wav_handle_free(wav_handle_t *h)
{
    h->clean_ctx(h);
    free(h);
}

int wav_parse_header(wav_handle_t *h)
{
    wav_header_t header;

    // Read full header
    if (h->read(h, &header, sizeof(header)) != sizeof(header)) {
        ESP_LOGE(TAG, "header read failed");
        return -1;
    }

    // Validate chunk IDs using memcmp (endianness-safe)
    if (memcmp(header.riff_header, "RIFF", 4) != 0) {
        ESP_LOGE(TAG, "riff_header not found");
        return -1;
    }

    if (memcmp(header.wave_header, "WAVE", 4) != 0) {
        ESP_LOGE(TAG, "wave_header not found");
        return -1;
    }

    if (memcmp(header.fmt_header, "fmt ", 4) != 0) {
        ESP_LOGE(TAG, "fmt_header not found");
        return -1;
    }

    if (header.audio_format != 1) { // PCM
        ESP_LOGE(TAG, "bad audio_format: %" PRIu16, header.audio_format);
        return -1;
    }

    if (header.sample_rate < 8000 || header.sample_rate > 44100) {
        ESP_LOGE(TAG, "bad sample_rate = %" PRIu32, header.sample_rate);
        return -1;
    }

    if (memcmp(header.data_header, "data", 4) != 0) {
        ESP_LOGE(TAG, "data_header not found");
        return -1;
    }

    ESP_LOGD(TAG, "num_channels=%" PRIu16, header.num_channels);
    ESP_LOGD(TAG, "sample_rate=%" PRIu32, header.sample_rate);
    ESP_LOGD(TAG, "byte_rate=%" PRIu32, header.byte_rate);
    ESP_LOGD(TAG, "sample_alignment=%" PRIu16, header.sample_alignment);
    ESP_LOGD(TAG, "bit_depth=%" PRIu16, header.bit_depth);
    ESP_LOGD(TAG, "data_bytes=%" PRIu32, header.data_bytes);

    h->num_channels = header.num_channels;
    h->sample_rate = header.sample_rate;
    h->byte_rate = header.byte_rate;
    h->sample_alignment = header.sample_alignment;
    h->bit_depth = header.bit_depth;
    h->data_start = sizeof(wav_header_t) + 8; //8 bytes of RIFF metadata
    h->data_bytes = header.data_bytes;
    return h->seek(h, h->data_start);
}
