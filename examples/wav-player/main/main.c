/* Simple WAV playback example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_system.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <driver/gpio.h>
#include <driver/i2s.h>

#include <esp_wav_player.h>

static const char *TAG="APP";

static xQueueHandle gpio_evt_queue = NULL;
esp_wav_player_t *esp_wav_player;

extern const uint8_t _binary_darude_wav_start[];
static wav_obj_t wav_example = {
	.type = WAV_EMBED,
	.data.embed.addr = _binary_darude_wav_start
};

esp_err_t audio_init(void)
{
	i2s_pin_config_t i2s_pin_config = {
		.bck_o_en = 1,
		.ws_o_en = 1,
		.data_out_en = 1,
	};

	i2s_config_t i2s_config = {
		.mode = I2S_MODE_MASTER | I2S_MODE_TX,  // Only TX
		.sample_rate = 22050,
		.bits_per_sample = 16,
		.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // 2-channels
		.communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_LSB,
		.dma_buf_count = 4,
		.dma_buf_len = 256,
		.tx_desc_auto_clear = 1
	};

	if(esp_wav_player)
		return ESP_ERR_INVALID_STATE;

	esp_wav_player = malloc(sizeof(esp_wav_player_t));
	if(!esp_wav_player)
		return ESP_ERR_NO_MEM;

	esp_wav_player->i2s_port = I2S_NUM_0;
	esp_wav_player->queue_len = 4;
	esp_wav_player->task_priority = 1,
	esp_wav_player->tda1543_mode = 0;
	esp_wav_player->has_amp_pwr_ctl = 0;

	esp_wav_player_init(esp_wav_player,&i2s_pin_config,&i2s_config);
	ESP_LOGI(TAG,"audio initialized");
	return ESP_OK;
}

static void gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void setup_gpio_interrupt(void)
{
	gpio_config_t io_conf;

	io_conf.intr_type = GPIO_INTR_NEGEDGE;
	io_conf.pin_bit_mask = (1 << GPIO_NUM_0);
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);

	//create a queue to handle gpio event from isr
	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

	gpio_install_isr_service(0);
	gpio_isr_handler_add(GPIO_NUM_0, gpio_isr_handler, (void *) GPIO_NUM_0);
}

void app_main()
{
	uint32_t io_num;
	bool playing;
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	ESP_LOGI(TAG, "WAV player demo. Press GPIO0 to play WAV");
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	audio_init();
	setup_gpio_interrupt();
	for (;;) {
		if (xQueueReceive(gpio_evt_queue, &io_num, 10)) {
			esp_wav_player_get_play_state(esp_wav_player,&playing);
			if(!playing){
				ESP_LOGI(TAG, "wav player start");
				esp_wav_player_play(esp_wav_player,&wav_example);
			}
		}
		vTaskDelay(250 / portTICK_RATE_MS);
	}
}
