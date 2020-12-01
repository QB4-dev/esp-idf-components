#include "esp_wav_player.h"
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/param.h>

#include <freertos/task.h>
#include <esp_log.h>
#include <esp_err.h>

#include "wav_header.h"

#define PLAYER_ACCESS_TIMEOUT 1000 //ms
#define PLAYER_DEFAULT_TASK_PRIO 5

#define AUDIO_BUF_LEN 2048
#define FCC(a,b,c,d)  ((uint32_t) (( (d)<<24) | ((c)<<16) | ((b)<<8) | (a) )) /* FourCC */
#define	LD_DWORD(ptr) (uint32_t)(*(uint32_t*)(uint8_t*)(ptr))
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

esp_err_t esp_wav_player_init(esp_wav_player_t *player)
{
	player->mutex = xSemaphoreCreateMutex();
	if (!player->mutex){
		ESP_LOGE(TAG, "Could not create player mutex");
		return ESP_FAIL;
	}
	SEMAPHORE_TAKE(player);
	ESP_ERROR_CHECK(i2s_driver_install(player->i2s_port, &player->i2s_config, 0, NULL));
	ESP_ERROR_CHECK(i2s_set_pin(player->i2s_port, &player->i2s_pin_config));
	if(!player->play_task_priority)
		player->play_task_priority = PLAYER_DEFAULT_TASK_PRIO;
	player->volume = 100;
	player->is_playing = false;
	SEMAPHORE_GIVE(player);
	return ESP_OK;
}

esp_err_t esp_wav_player_deinit(esp_wav_player_t *player)
{
	SEMAPHORE_TAKE(player);
	ESP_ERROR_CHECK(i2s_zero_dma_buffer(player->i2s_port));
	ESP_ERROR_CHECK(i2s_stop(player->i2s_port));
	ESP_ERROR_CHECK(i2s_driver_uninstall(player->i2s_port));
	SEMAPHORE_GIVE(player);
	vSemaphoreDelete(player->mutex);
	return ESP_OK;
}

esp_err_t esp_wav_player_is_playing(esp_wav_player_t *player, bool *state)
{
	if(!state)
		return ESP_ERR_INVALID_ARG;
	SEMAPHORE_TAKE(player);
	*state = player->is_playing;
	SEMAPHORE_GIVE(player);
	return ESP_OK;
}

esp_err_t esp_wav_player_set_volume(esp_wav_player_t *player, uint8_t vol)
{
	SEMAPHORE_TAKE(player);
	player->volume = vol;
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
	props->num_channels = header->num_channels;
	props->bit_depth = header->bit_depth;
	props->sample_rate = header->sample_rate;
	props->sample_alignment = header->sample_alignment;
	props->data_bytes = header->data_bytes;
	return true;
}

static void i2s_set_volume( int8_t vol, wav_obj_t *wav, uint8_t *buf, size_t len)
{
	uint8_t  *sample_8bit;
	int16_t  *sample_16bit;

	switch(wav->props.bit_depth){
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

static void i2s_tda_compat(wav_obj_t *wav, uint8_t *buf, size_t len)
{
	int16_t  *sample_16bit;

	switch(wav->props.bit_depth){
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

static int i2s_play_wav(esp_wav_player_t *player)
{
	i2s_port_t i2s_port;
	wav_obj_t *wav;
	uint8_t *audio_buf;
	uint8_t *audio_ptr;
	uint32_t in_buf;
	uint32_t div;
	size_t bytes_left;
	//those may change during play
	bool keep_playing;
	uint8_t volume;

	SEMAPHORE_TAKE(player);
	assert(player->wav);
	wav = player->wav;
	i2s_port = player->i2s_port;
	player->is_playing = true;
	SEMAPHORE_GIVE(player);

	div = (wav->props.sample_alignment == 2)?2:1;
	bytes_left = wav->props.data_bytes;

	//prepare buffer
	audio_buf = malloc(AUDIO_BUF_LEN);
	if(!audio_buf){
		ESP_LOGE(TAG, "audio_buf malloc error");
		return ESP_ERR_NO_MEM;
	}

	i2s_set_clk(i2s_port, wav->props.sample_rate/div, wav->props.bit_depth, wav->props.num_channels);
	i2s_start(i2s_port);
	while(bytes_left > 0){
		SEMAPHORE_TAKE(player);
		keep_playing = player->is_playing;
		volume = player->volume;
		SEMAPHORE_GIVE(player);
		if(!keep_playing)
			break;

		in_buf = wav_object_read(wav,audio_buf,MIN(bytes_left,AUDIO_BUF_LEN));
		audio_ptr = audio_buf;
		bytes_left -= in_buf;

		i2s_set_volume(volume,wav,audio_buf,AUDIO_BUF_LEN);
		if(player->tda_1543_mode)
			i2s_tda_compat(wav,audio_buf,AUDIO_BUF_LEN);

		for(size_t i2s_wr = 0;audio_ptr-audio_buf < in_buf;){
			i2s_write(i2s_port,audio_ptr,in_buf,&i2s_wr,100);
			audio_ptr += i2s_wr;
		}
	}
	wav_object_close(wav);
	free(audio_buf);

	SEMAPHORE_TAKE(player);
	player->is_playing = false;
	SEMAPHORE_GIVE(player);
	return ESP_OK;
}

static void wav_play_task(void *arg)
{
	esp_wav_player_t *player = arg;
	assert(player);
	i2s_play_wav(player);
	vTaskDelete(NULL);
}

esp_err_t esp_wav_player_play(esp_wav_player_t *player, wav_obj_t *wav)
{
	uint8_t header_buf[sizeof(wav_header_t)];
	bool playing;

	ESP_LOGD(TAG, "play wav...");
	ESP_ERROR_CHECK(esp_wav_player_is_playing(player,&playing));
	if(playing){
		ESP_LOGW(TAG, "already playing");
		return ESP_FAIL;
	}

	if(!wav_object_open(wav)){
		ESP_LOGE(TAG, "wav_object_open");
		return ESP_FAIL;
	}

	if( wav_object_read(wav, header_buf, sizeof(wav_header_t)) < 0){
		ESP_LOGE(TAG, "wav_object_read");
		wav_object_close(wav);
		return ESP_FAIL;
	}

	if(!esp_decode_wav_header(header_buf, &wav->props))
		return ESP_FAIL;

	SEMAPHORE_TAKE(player);
	player->wav = wav;
	xTaskCreate(wav_play_task, "wav_play", 1024, player, player->play_task_priority, NULL);
	SEMAPHORE_GIVE(player);
	return ESP_OK;
}

esp_err_t esp_wav_player_stop(esp_wav_player_t *player)
{
	SEMAPHORE_TAKE(player);
	player->is_playing = false;
	SEMAPHORE_GIVE(player);
	return ESP_OK;
}
