/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_system.h>
#include <esp_log.h>
#include <esp_event_loop.h>
#include <tcpip_adapter.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <driver/gpio.h>
#include <driver/i2s.h>

#include <esp_wav_player.h>

#include "../wav/include/darude.h"

static const char *TAG="APP";

static xQueueHandle gpio_evt_queue = NULL;

static esp_wav_player_t wav_player = {
	.i2s_port = I2S_NUM_0,
	.queue_len = 3,
};

static i2s_pin_config_t i2s_pin_conf = {
	.bck_o_en = 1,
	.ws_o_en = 1,
	.data_out_en = 1,
};

static i2s_config_t i2s_conf = {
	.mode = I2S_MODE_MASTER | I2S_MODE_TX,  // Only TX
	.sample_rate = 11025,
	.bits_per_sample = 16,
	.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // 2-channels
	.communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_LSB,
	.dma_buf_count = 2,
	.dma_buf_len = 512,
	.tx_desc_auto_clear = 1
};



static wav_obj_t wav_example = {
	.type = WAV_EMBED,
	.data.embed.addr = darude_wav
};

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

	ESP_LOGI(TAG, "WAV player demo. Press GPIO0 to play WAV");
	ESP_ERROR_CHECK(esp_wav_player_init(&wav_player,&i2s_pin_conf,&i2s_conf));
	ESP_ERROR_CHECK(esp_wav_player_set_volume(&wav_player,50));
	setup_gpio_interrupt();
	for (;;) {
		if (xQueueReceive(gpio_evt_queue, &io_num, 10)) {
			ESP_ERROR_CHECK(esp_wav_player_get_play_state(&wav_player,&playing));
			if(!playing){
				ESP_LOGI(TAG, "wav player start");
				esp_wav_player_play(&wav_player,&wav_example);
			}
		}
		vTaskDelay(100 / portTICK_RATE_MS);
	}
}
