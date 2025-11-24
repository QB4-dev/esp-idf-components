# esp_wav_player
Simple wav player component for ESP-IDF / ESP8266_RTOS_SDK

esp_wav_player is a lightweight component for ESP32 and ESP8266 that plays PCM WAV files from local storage or streamed sources. It supports common PCM WAV formats (8-bit and 16-bit), mono and stereo channels, and typical sample rates (8 kHz–48 kHz). The component provides a simple API to initialize the player/I2S, start/stop playback, and feed WAV data from SPIFFS, SD card (FAT), or network streams.

Features

- Supports PCM WAV (8-bit and 16-bit), mono and stereo
- Plays files from SPIFFS/FAT or from streams
- Simple playback API: initialize, play, pause, stop
- Works with ESP-IDF and ESP8266_RTOS_SDK
- Example project included in the examples/ directory

Basic usage

1. Add esp_wav_player as a component in your project (components/esp_wav_player) and include header
```c
#include <esp_wav_player.h>
```
2. Initialize I2S and the wav player with your configuration:
```c
esp_wav_player_config_t player_conf = ESP_WAV_PLAYER_DEFAULT_CONFIG();
esp_wav_player_t        wav_player;

esp_wav_player_init(&wav_player, &player_conf);
esp_wav_player_set_volume(wav_player, 50); // set initial volume
```
> [!NOTE]  
> On ESP32 remember to set proper gpio pinout for i2s DAC connections in `player_conf`

3. Add .wav files to your project:
    #### Files embedded in program memory
    Use **EMBED_TXTFILES** in CMakeLists.txt and list files to embed
    
    ```cmake
    idf_component_register(SRCS "main.c"
                  INCLUDE_DIRS "."
                  EMBED_TXTFILES "../wav/darude.wav")
    ```
    
    Then use binary in program and **WAV_DECLARE_EMBED** macro:
    ```c
    extern const uint8_t _binary_darude_wav_start[];
    WAV_DECLARE_EMBED(wav_example, _binary_darude_wav_start);
    ```
    
    #### Files in SPIFFS filesystem
    
    To use file from SPIFFS use **WAV_DECLARE_SPIFFS** macro:
    ```c
    WAV_DECLARE_SPIFFS(wav_example, "/spiffs/audio.wav");
    ```
  
    #### Files from MMC card with FATFS
    
    To use file from MMC use **WAV_DECLARE_MMC** macro:
    ```c
    WAV_DECLARE_MMC(wav_example, "/sdcard/audio.wav");
    ```
   
4. Start playback using play function. 
  ```c
  esp_wav_player_play(wav_player, &wav_example);
  ```
  > [!NOTE]  
  > Files are queued and played in background, so you can put multiple files one by one  

5. Stop or pause playback as needed.

    See examples/default/README.md for example pin mappings and a quickstart for ESP32/ESP8266.

## Installation

### Using ESP Component Registry

[![Component Registry](https://components.espressif.com/components/qb4-dev/esp_wav_player/badge.svg)](https://components.espressif.com/components/qb4-dev/esp_wav_player)

```bash
idf.py add-dependency "qb4-dev/esp_wav_player=*"
```

### Manual Installation

Clone this repository into your project's `components` directory:

```bash
cd your_project/components
git clone https://github.com/QB4-dev/esp_wav_player.git esp_wav_player
```


