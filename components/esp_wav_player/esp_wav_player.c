#include "esp_wav_player.h"
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/param.h>

#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_log.h>
#include <esp_err.h>

#include "wav_header.h"

#define PLAYER_ACCESS_TIMEOUT 1000 //ms
#define PLAYER_DEFAULT_TASK_PRIO 0

#define AUDIO_BUF_LEN 1024
#define FCC(a,b,c,d)  ((uint32_t) (( (d)<<24) | ((c)<<16) | ((b)<<8) | (a) )) /* FourCC */
#define LD_DWORD(ptr) (uint32_t)(*(uint32_t*)(uint8_t*)(ptr))
#define BSWAP(b)      ((b>>8)|(b<<8)) //for TDA 1543 I2S frame

static const char *TAG="WAV";

#define SEMAPHORE_TAKE(player) do { \
	if (!xSemaphoreTake(player->mutex, PLAYER_ACCESS_TIMEOUT / portTICK_RATE_MS)){ \
		ESP_LOGE(TAG, "can't take player mutex"); \
		return ESP_ERR_TIMEOUT; \
	} \
	} while (0)

#define SEMAPHORE_GIVE(player) do { \
	if (!xSemaphoreGive(player->mutex)){ \
		ESP_LOGE(TAG, "can't give player mutex"); \
		return ESP_FAIL; \
	} \
	} while (0)

esp_err_t esp_wav_player_init(esp_wav_player_t *player, i2s_pin_config_t *i2s_pin_conf, i2s_config_t *i2s_conf)
{
	gpio_config_t io_conf = {0};

	player->mutex = xSemaphoreCreateMutex();
	if (!player->mutex){
		ESP_LOGE(TAG, "Could not create player mutex");
		return ESP_ERR_NO_MEM;
	}
	ESP_ERROR_CHECK(i2s_driver_install(player->i2s_port, i2s_conf, 0, NULL));
	ESP_ERROR_CHECK(i2s_set_pin(player->i2s_port, i2s_pin_conf));

	player->volume = 100;
	player->is_playing = false;

	if(!player->task_priority)
		player->task_priority = PLAYER_DEFAULT_TASK_PRIO;

	if(player->queue_len == 0){
		ESP_LOGW(TAG, "player queue_len not set!");
		player->queue_len = 1;
	}
	player->queue = xQueueCreate(player->queue_len,sizeof(wav_obj_t));
	if(!player->queue){
		ESP_LOGE(TAG, "cannot create player queue");
		return ESP_ERR_NO_MEM;
	}
	player->event_group = xEventGroupCreate();
	if(!player->event_group){
		ESP_LOGE(TAG, "cannot create player event_group");
		return ESP_ERR_NO_MEM;
	}

	if(player->has_amp_pwr_ctl){
		io_conf.mode = GPIO_MODE_OUTPUT;
		io_conf.intr_type = GPIO_INTR_DISABLE;
		io_conf.pin_bit_mask = (1 << player->amp_power_gpio);
		ESP_ERROR_CHECK(gpio_config(&io_conf));
	}

	ESP_LOGD(TAG, "player init OK");
	return ESP_OK;
}

esp_err_t esp_wav_player_deinit(esp_wav_player_t *player)
{
	SEMAPHORE_TAKE(player);
	ESP_ERROR_CHECK(i2s_zero_dma_buffer(player->i2s_port));
	ESP_ERROR_CHECK(i2s_stop(player->i2s_port));
	ESP_ERROR_CHECK(i2s_driver_uninstall(player->i2s_port));
	if(player->queue)
		vQueueDelete(player->queue);

	if(player->event_group)
		vEventGroupDelete(player->event_group);
	SEMAPHORE_GIVE(player);
	vSemaphoreDelete(player->mutex);
	return ESP_OK;
}

esp_err_t esp_wav_player_get_play_state(esp_wav_player_t *player, bool *state)
{
	if(!player || !state)
		return ESP_ERR_INVALID_ARG;
	SEMAPHORE_TAKE(player);
	*state = player->is_playing;
	SEMAPHORE_GIVE(player);
	return ESP_OK;
}

static esp_err_t esp_wav_player_set_play_state(esp_wav_player_t *player, bool state)
{
	SEMAPHORE_TAKE(player);
	if(state == true){
		xEventGroupSetBits(player->event_group, ESP_WAV_PLAYER_STARTED);
		xEventGroupClearBits(player->event_group,ESP_WAV_PLAYER_STOPPED);
	} else {
		xEventGroupSetBits(player->event_group, ESP_WAV_PLAYER_STOPPED);
		xEventGroupClearBits(player->event_group, ESP_WAV_PLAYER_STARTED);
	}
	player->is_playing = state;
	SEMAPHORE_GIVE(player);
	return ESP_OK;
}

esp_err_t esp_wav_player_set_volume(esp_wav_player_t *player, uint8_t vol)
{
	if(!player)
		return ESP_ERR_INVALID_ARG;

	SEMAPHORE_TAKE(player);
	player->volume = vol;
	SEMAPHORE_GIVE(player);
	return ESP_OK;
}

esp_err_t esp_wav_player_get_volume(esp_wav_player_t *player, uint8_t *vol)
{
	if(!player || !vol)
		return ESP_ERR_INVALID_ARG;

	SEMAPHORE_TAKE(player);
	*vol = player->volume;
	SEMAPHORE_GIVE(player);
	return ESP_OK;
}

static esp_err_t esp_wav_player_amp_power_ctl(esp_wav_player_t *player, bool amp_enable)
{
	SEMAPHORE_TAKE(player);
	if(amp_enable == true){
		gpio_set_level(player->amp_power_gpio,1);
		vTaskDelay(player->amp_power_on_delay/portTICK_RATE_MS);
	} else {
		vTaskDelay(player->amp_power_off_delay/portTICK_RATE_MS);
		gpio_set_level(player->amp_power_gpio,0);
	}
	SEMAPHORE_GIVE(player);
	return ESP_OK;
}

static bool esp_decode_wav_header(uint8_t *buf, wav_properties_t *props)
{
	wav_header_t *header = (wav_header_t*)buf;

	if(LD_DWORD(header->riff_header) != FCC('R','I','F','F')){
		ESP_LOGE(TAG, "riff_header not found");
		return false;
	}
	ESP_LOGD(TAG, "wav file size=%d", header->wav_size);

	if(LD_DWORD(header->wave_header) != FCC('W','A','V','E')){
		ESP_LOGE(TAG, "wave_header not found");
		return false;
	}

	if(LD_DWORD(header->fmt_header) != FCC('f','m','t',' ')){
		ESP_LOGE(TAG, "fmt_header not found");
		return false;
	}

	if(header->audio_format != 0x01){ // 0x01 = PCM
		ESP_LOGE(TAG, "bad audio_format");
		return false;
	}

	ESP_LOGD(TAG, "num_channels=%d", header->num_channels);
	ESP_LOGD(TAG, "sample_rate=%d", header->sample_rate);
	ESP_LOGD(TAG, "byte_rate=%d", header->byte_rate);
	ESP_LOGD(TAG, "sample_alignment=%d", header->sample_alignment);
	ESP_LOGD(TAG, "bit_depth=%d", header->bit_depth);

	if( header->sample_rate < 8000 || header->sample_rate > 44100){
		ESP_LOGE(TAG, "bad sample_rate = %d", header->sample_rate);
		return false;
	}

	if(LD_DWORD(header->data_header) != FCC('d','a','t','a') ){
		ESP_LOGE(TAG, "data_header not found");
		return false;
	}
	ESP_LOGD(TAG, "data_bytes=%d", header->data_bytes);

	//set properties
	if(props != NULL){
		props->num_channels = header->num_channels;
		props->bit_depth = header->bit_depth;
		props->sample_rate = header->sample_rate;
		props->sample_alignment = header->sample_alignment;
		props->data_bytes = header->data_bytes;
	}
	return true;
}

static void i2s_volume_control(int8_t vol, wav_properties_t *wav_props, uint8_t *buf, size_t len)
{
	uint8_t  *sample_8bit;
	int16_t  *sample_16bit;

	switch(wav_props->bit_depth){
	case 8:
		sample_8bit = buf;
		for(int i=0; i<len; i++){
			*sample_8bit = (int32_t)(*sample_8bit * vol)/100;
			sample_8bit++;
		}
		break;
	case 16:
		sample_16bit = (int16_t *)buf;
		for(int i=0; i<len/2; i++){
			*sample_16bit = (int32_t)(*sample_16bit * vol)/100;
			sample_16bit++;
		}
		break;
	default:
		break;
	}
}

static void i2s_tda_compat(wav_properties_t *wav_props, uint8_t *buf, size_t len)
{
	int16_t  *sample_16bit;

	switch(wav_props->bit_depth){
	case 8:
		break;
	case 16:
		sample_16bit = (int16_t *)buf;
		for(int i=0; i<len/2; i++){
			*sample_16bit = BSWAP(*sample_16bit);
			sample_16bit++;
		}
		break;
	default:
		break;
	}
}

static esp_err_t i2s_play_wav(esp_wav_player_t *player, wav_obj_t *wav)
{
	uint8_t header_buf[sizeof(wav_header_t)];
	wav_properties_t wav_props;
	wav_handle_t wavh;

	i2s_port_t i2s_port;
	uint8_t *audio_buf;
	uint8_t *audio_ptr;
	int in_buf;
	int div;
	size_t bytes_left;
	//those may change during play
	bool keep_playing;
	uint8_t volume;

	if(!wav)
		return ESP_ERR_INVALID_ARG;

	if(!wav_object_open(wav,&wavh)){
		ESP_LOGE(TAG, "wav_obj open error");
		return ESP_FAIL;
	}

	if( wav_object_read(&wavh, header_buf, sizeof(wav_header_t)) < 0){
		ESP_LOGE(TAG, "wav_obj read error");
		wav_object_close(&wavh);
		return ESP_FAIL;
	}

	if(!esp_decode_wav_header(header_buf, &wav_props)){
		wav_object_close(&wavh);
		return ESP_FAIL;
	}

	div = (wav_props.sample_alignment == 2)?2:1;
	bytes_left = wav_props.data_bytes;

	//prepare buffer
	audio_buf = malloc(AUDIO_BUF_LEN);
	if(!audio_buf){
		ESP_LOGE(TAG, "audio buf malloc error");
		return ESP_ERR_NO_MEM;
	}
	esp_wav_player_set_play_state(player,true);
	SEMAPHORE_TAKE(player);
	i2s_port = player->i2s_port;
	i2s_set_clk(i2s_port, wav_props.sample_rate/div, wav_props.bit_depth, wav_props.num_channels);
	i2s_start(i2s_port);
	SEMAPHORE_GIVE(player);

	while(bytes_left > 0){
		esp_wav_player_get_play_state(player,&keep_playing);
		esp_wav_player_get_volume(player,&volume);

		if(!keep_playing)
			break;

		in_buf = wav_object_read(&wavh,audio_buf,MIN(bytes_left,AUDIO_BUF_LEN));
		if(in_buf < 0){
			ESP_LOGE(TAG, "wav obj read error");
			break;
		}

		audio_ptr = audio_buf;
		bytes_left -= in_buf;

		i2s_volume_control(volume,&wav_props,audio_buf,AUDIO_BUF_LEN);
		if(player->tda1543_mode)
			i2s_tda_compat(&wav_props,audio_buf,AUDIO_BUF_LEN);

		for(size_t i2s_wr = 0;audio_ptr-audio_buf < in_buf;){
			i2s_write(i2s_port,audio_ptr,in_buf,&i2s_wr,500/portTICK_RATE_MS);
			audio_ptr += i2s_wr;
		}
	}
	wav_object_close(&wavh);
	free(audio_buf);
	//AUDIO_BUF_LEN/wav_props.sample_rate

	vTaskDelay(300/portTICK_RATE_MS); //TODO calculate depending on sampling rate
	//i2s_zero_dma_buffer(i2s_port);
	return ESP_OK;
}

static void wav_play_queue_task(void *arg)
{
	esp_wav_player_t *player = arg;
	wav_obj_t wav_obj;

	if(player->has_amp_pwr_ctl)
		esp_wav_player_amp_power_ctl(player,true);

	ESP_LOGD(TAG, "queue start");
	while(xQueueReceive(player->queue,&wav_obj,1000/portTICK_RATE_MS)){
		i2s_play_wav(player,&wav_obj);
	}
	ESP_LOGD(TAG, "queue end");

	if(player->has_amp_pwr_ctl)
		esp_wav_player_amp_power_ctl(player,false);

	esp_wav_player_set_play_state(player,false);
	vTaskDelete(NULL);
}

static esp_err_t wav_is_valid(wav_obj_t *wav)
{
	uint8_t header_buf[sizeof(wav_header_t)];
	wav_handle_t wavh;

	if(!wav_object_open(wav,&wavh)){
		ESP_LOGE(TAG, "invalid wav_obj open error");
		return ESP_FAIL;
	}

	if( wav_object_read(&wavh, header_buf, sizeof(wav_header_t)) < 0){
		ESP_LOGE(TAG, "invalid wav_obj read error");
		wav_object_close(&wavh);
		return ESP_FAIL;
	}
	wav_object_close(&wavh);

	if(!esp_decode_wav_header(header_buf, NULL))
		return ESP_FAIL;
	else
		return ESP_OK;
}

esp_err_t esp_wav_player_add_to_queue(esp_wav_player_t *player, wav_obj_t *wav)
{
	int rc = ESP_OK;

	if(!player || !wav)
		return ESP_ERR_INVALID_ARG;

	if(!!wav_is_valid(wav))
		return ESP_FAIL;

	SEMAPHORE_TAKE(player);
	if(xQueueSend(player->queue,wav,100/portTICK_RATE_MS) == errQUEUE_FULL){
		ESP_LOGE(TAG, "queue full");
		rc = ESP_ERR_NO_MEM;
	}
	SEMAPHORE_GIVE(player);
	return rc;
}

esp_err_t esp_wav_player_get_queued(esp_wav_player_t *player, uint8_t *queued)
{
	if(!player || !queued)
		return ESP_ERR_INVALID_ARG;

	SEMAPHORE_TAKE(player);
	*queued = uxQueueMessagesWaiting(player->queue);
	SEMAPHORE_GIVE(player);
	return ESP_OK;
}

esp_err_t esp_wav_player_reset_queue(esp_wav_player_t *player)
{
	if(!player->queue)
		return ESP_ERR_NOT_FOUND;

	xQueueReset(player->queue);
	return ESP_OK;
}

esp_err_t esp_wav_player_play_queued(esp_wav_player_t *player)
{
	bool playing;
	uint8_t queued;

	esp_wav_player_get_play_state(player,&playing);
	if(playing){
		ESP_LOGW(TAG, "already playing");
		return ESP_ERR_INVALID_STATE;
	}

	esp_wav_player_get_queued(player,&queued);
	if(queued == 0)
		return ESP_ERR_NOT_FOUND;

	if(xTaskCreate(wav_play_queue_task, "wav_play", 2048, player, player->task_priority, NULL) != pdPASS){
		ESP_LOGE(TAG, "cannot create play task");
		return ESP_FAIL;
	}
	return ESP_OK;
}

esp_err_t esp_wav_player_play(esp_wav_player_t *player, wav_obj_t *wav)
{
	esp_err_t rc;
	rc = esp_wav_player_add_to_queue(player,wav);
	if(rc != ESP_OK)
		return rc;
		
	return esp_wav_player_play_queued(player);
}

esp_err_t esp_wav_player_stop(esp_wav_player_t *player)
{
	esp_wav_player_reset_queue(player);
	esp_wav_player_set_play_state(player,false);
	return ESP_OK;
}
