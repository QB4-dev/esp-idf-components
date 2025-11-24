#include "include/esp_wav_player.h"
#include <string.h>
#include <esp_log.h>
#include "wav_handle.h"

#define WAV_BUF_SIZE 1024

static const char *TAG = "WAV";
static void        wav_player_task(void *arg);

struct esp_wav_player {
    QueueHandle_t queue;
    TaskHandle_t  task;

    i2s_config_t     base_cfg;
    i2s_pin_config_t pins;
    int              i2s_num;

    volatile esp_wav_player_state_t state;
    volatile bool                   stop_request;
    volatile bool                   pause_request;

    uint8_t volume;

    esp_wav_player_cb_t on_start;
    esp_wav_player_cb_t on_end;

    void *on_start_arg;
    void *on_end_arg;
};

esp_err_t esp_wav_player_init(esp_wav_player_t *hdl, const esp_wav_player_config_t *cfg)
{
    if (!hdl || !cfg)
        return ESP_ERR_INVALID_ARG;

    struct esp_wav_player *player = calloc(1, sizeof(*player));
    if (!player)
        return ESP_ERR_NO_MEM;

    player->queue = xQueueCreate(cfg->queue_len, sizeof(wav_handle_t *));
    if (!player->queue) {
        free(player);
        return ESP_FAIL;
    }

    player->volume = 100;
    player->state = ESP_WAV_PLAYER_STOPPED;

    player->i2s_num = cfg->i2s_num;
    player->pins = cfg->i2s_pin_config;
    player->base_cfg = cfg->base_cfg;

    i2s_driver_install(player->i2s_num, &player->base_cfg, 0, NULL);
    i2s_set_pin(player->i2s_num, &player->pins);

    xTaskCreate(wav_player_task, "wav_player_task", 4096, player, 5, &player->task);

    *hdl = player;
    return ESP_OK;
}

esp_err_t esp_wav_player_deinit(esp_wav_player_t hdl)
{
    if (!hdl)
        return ESP_ERR_INVALID_ARG;

    struct esp_wav_player *player = (struct esp_wav_player *)hdl;

    // Tell task to exit
    player->state = ESP_WAV_PLAYER_STOPPED;

    // Wake the task in case it's blocking on queue
    wav_handle_t *dummy = NULL;
    xQueueSend(player->queue, &dummy, 0);

    // Wait for the task to exit
    if (player->task) {
        vTaskDelete(player->task);
        player->task = NULL;
    }

    // Uninstall I2S driver
    i2s_driver_uninstall(player->i2s_num);

    // Delete queue
    if (player->queue) {
        xQueueReset(player->queue);
        vQueueDelete(player->queue);
        player->queue = NULL;
    }

    free(player);
    return ESP_OK;
}

esp_err_t esp_wav_player_play(esp_wav_player_t hdl, const wav_obj_t *src)
{
    if (!hdl || !src)
        return ESP_ERR_INVALID_ARG;

    struct esp_wav_player *player = (struct esp_wav_player *)hdl;
    wav_handle_t          *h = wav_handle_init(src);
    if (!h)
        return ESP_FAIL;

    return xQueueSend(player->queue, &h, 0) == pdTRUE ? ESP_OK : ESP_FAIL;
}

esp_err_t esp_wav_player_stop(esp_wav_player_t hdl)
{
    if (!hdl)
        return ESP_ERR_INVALID_ARG;

    struct esp_wav_player *player = (struct esp_wav_player *)hdl;
    player->stop_request = true;
    return ESP_OK;
}

esp_err_t esp_wav_player_pause(esp_wav_player_t hdl)
{
    if (!hdl)
        return ESP_ERR_INVALID_ARG;

    struct esp_wav_player *player = (struct esp_wav_player *)hdl;
    player->pause_request = !player->pause_request;
    return ESP_OK;
}

esp_err_t esp_wav_player_get_state(esp_wav_player_t hdl, esp_wav_player_state_t *st)
{
    if (!hdl || !st)
        return ESP_ERR_INVALID_ARG;

    struct esp_wav_player *player = (struct esp_wav_player *)hdl;
    *st = player->state;
    return ESP_OK;
}

esp_err_t esp_wav_player_set_volume(esp_wav_player_t hdl, uint8_t vol)
{
    if (!hdl)
        return ESP_ERR_INVALID_ARG;

    struct esp_wav_player *player = (struct esp_wav_player *)hdl;
    player->volume = vol;
    return ESP_OK;
}

esp_err_t esp_wav_player_get_volume(esp_wav_player_t hdl, uint8_t *vol)
{
    if (!hdl || !vol)
        return ESP_ERR_INVALID_ARG;

    struct esp_wav_player *player = (struct esp_wav_player *)hdl;
    *vol = player->volume;
    return ESP_OK;
}

esp_err_t esp_wav_player_get_queued(esp_wav_player_t hdl, size_t *qlen)
{
    if (!hdl || !qlen)
        return ESP_ERR_INVALID_ARG;

    struct esp_wav_player *player = (struct esp_wav_player *)hdl;
    *qlen = uxQueueMessagesWaiting(player->queue);
    return ESP_OK;
}

void esp_wav_player_set_start_cb(esp_wav_player_t hdl, esp_wav_player_cb_t cb, void *arg)
{
    if (!hdl)
        return;

    struct esp_wav_player *player = (struct esp_wav_player *)hdl;
    player->on_start = cb;
    player->on_start_arg = arg;
}

void esp_wav_player_set_end_cb(esp_wav_player_t hdl, esp_wav_player_cb_t cb, void *arg)
{
    if (!hdl)
        return;

    struct esp_wav_player *player = (struct esp_wav_player *)hdl;
    player->on_end = cb;
    player->on_end_arg = arg;
}

static void wav_player_task(void *arg)
{
    struct esp_wav_player *player = arg;
    wav_handle_t          *wavh = NULL;

    uint8_t buf[WAV_BUF_SIZE];
    size_t  i2s_wr;
    while (1) {
        if (!xQueueReceive(player->queue, &wavh, portMAX_DELAY))
            continue;

        // Check if we received a valid handle or stop signal
        if (wavh == NULL)
            break;

        if (wavh->open(wavh) != 0) {
            wavh->close(wavh);
            player->state = ESP_WAV_PLAYER_STOPPED;
            ESP_LOGE(TAG, "wav open failed");
            continue;
        }
        if (wav_parse_header(wavh) != 0) {
            wavh->close(wavh);
            player->state = ESP_WAV_PLAYER_STOPPED;
            continue;
        }

        player->state = ESP_WAV_PLAYER_PLAYING;
        player->stop_request = false;
        player->pause_request = false;

        if (player->on_start)
            player->on_start(player, player->on_start_arg);

        float vol = player->volume / 100.0f;
        int   bytes_left = wavh->data_bytes;

        i2s_set_clk(player->i2s_num, wavh->sample_rate, wavh->bit_depth, wavh->num_channels);
        while (!player->stop_request) {
            if (player->pause_request) {
                player->state = ESP_WAV_PLAYER_PAUSED;
                vTaskDelay(10);
                continue;
            }
            if (bytes_left <= 0)
                break;

            size_t n = wavh->read(wavh, buf, WAV_BUF_SIZE);
            if (n == 0)
                break;

            switch (wavh->bit_depth) {
            case 8: {
                uint8_t *sample = buf;
                for (size_t i = 0; i < n; i++) {
                    int16_t v = ((int16_t)sample[i] - 128);
                    v = v * vol;
                    v += 128;
                    if (v < 0)
                        v = 0;
                    if (v > 255)
                        v = 255;
                    sample[i] = (uint8_t)v;
                }
            } break;
            case 16: {
                int16_t *sample = (int16_t *)buf;
                size_t   count = n / 2;
                for (size_t i = 0; i < count; i++)
                    sample[i] = (int16_t)(sample[i] * vol);
            } break;
            default:
                break;
            }
            i2s_write(player->i2s_num, buf, n, &i2s_wr, portMAX_DELAY);
            bytes_left -= i2s_wr;
        }
        i2s_zero_dma_buffer(player->i2s_num);
        wavh->close(wavh);
        wav_handle_free(wavh);
        if (player->on_end)
            player->on_end(player, player->on_end_arg);

        player->state = ESP_WAV_PLAYER_STOPPED;
    }
}
