#include "esp_wav_object.h"
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <esp_err.h>

bool wav_object_open(wav_obj_t *wav)
{
	switch(wav->type){
		case WAV_PROGMEM:
			wav->data.embed.read_ptr = wav->data.embed.file;
			return true;
		case WAV_SPIFFS:
			wav->data.spiffs.fd = open(wav->data.spiffs.file,O_RDONLY);
			if(wav->data.spiffs.fd == -1)
				return false;
			return true;
		default:
			break;
	}
	return false;
}

int wav_object_read(wav_obj_t *wav, void *buf, size_t count)
{
	switch(wav->type){
		case WAV_PROGMEM:
			memcpy(buf,wav->data.embed.read_ptr,count);
			wav->data.embed.read_ptr+=count; //move read ptr
			return count;
		case WAV_SPIFFS:
			return read(wav->data.spiffs.fd,buf,count);
		default:
			break;
	}
	return -1;
}

int wav_object_close(wav_obj_t *wav)
{
	switch(wav->type){
		case WAV_PROGMEM:
			wav->data.embed.read_ptr = wav->data.embed.file;
			return 0;
		case WAV_SPIFFS:
			close(wav->data.spiffs.fd);
			return 0;
		default:
			break;
	}
	return -1;
}
