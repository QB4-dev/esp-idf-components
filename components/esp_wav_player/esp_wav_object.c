#include "esp_wav_object.h"
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <esp_err.h>

bool wav_object_open(wav_obj_t *wav, wav_handle_t *wavh)
{
	switch(wav->type){
		case WAV_EMBED:{
			wavh->type = WAV_EMBED;
			wavh->io.embed_rdptr = wav->data.embed.addr;
			return true;
		}
		case WAV_SPIFFS:{
			int fd = open(wav->data.spiffs.path,O_RDONLY);
			if(fd == -1)
				return false;
			wavh->type = WAV_SPIFFS;
			wavh->io.spiffs_fd = fd;
			return true;
		}
		default:
			break;
	}
	return false;
}

int wav_object_read(wav_handle_t *wavh, void *buf, size_t count)
{
	switch(wavh->type){
		case WAV_EMBED:
			memcpy(buf,wavh->io.embed_rdptr,count);
			wavh->io.embed_rdptr += count; //move read ptr
			return count;
		case WAV_SPIFFS:
			return read(wavh->io.spiffs_fd,buf,count);
		default:
			break;
	}
	return -1;
}

int wav_object_close(wav_handle_t *wavh)
{
	switch(wavh->type){
		case WAV_EMBED:
			return 0;
		case WAV_SPIFFS:
			close(wavh->io.spiffs_fd);
			return 0;
		default:
			break;
	}
	return -1;
}
