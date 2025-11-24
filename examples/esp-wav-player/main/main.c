/* Simple WAV playback example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_wav_player.h>

static const char *TAG = "APP";

extern const uint8_t _binary_darude_wav_start[];
WAV_DECLARE_EMBED(wav_example, _binary_darude_wav_start);

void app_main()
{
    esp_wav_player_config_t player_conf = ESP_WAV_PLAYER_DEFAULT_CONFIG();
    esp_wav_player_t        wav_player;

    ESP_ERROR_CHECK(esp_event_loop_create_default());

#if CONFIG_IDF_TARGET_ESP8266
    /* for some reason i2s doesn't work on ESP8266 if wifi is not initialized */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
#endif

    esp_wav_player_init(&wav_player, &player_conf);
    esp_wav_player_set_volume(wav_player, 50);
    ESP_LOGI(TAG, "wav player start");
    esp_wav_player_play(wav_player, &wav_example);
}
