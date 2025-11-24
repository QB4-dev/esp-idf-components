# ESP WAV player example

Connect to ESP8266 I2S DAC like PCM5102 with pinout as below:

| pin name| function | gpio_num |
|:---:|:---:|:---:|
| LCK |word select| GPIO_NUM_2 |
| BCK | bit clock | GPIO_NUM_15 |
| DIN |serial data| GPIO_NUM_3 |

For ESP32, example I2S pin mappings (adjustable in your application) are shown below:

| pin name / struct field | function | gpio_num (example) |
|:---:|:---:|:---:|
| `.ws_io_num` (LCK / WS) | word select (WS) | `GPIO_NUM_25` |
| `.bck_io_num` (BCK) | bit clock | `GPIO_NUM_32` |
| `.data_out_num` (DIN / DOUT) | serial data (DOUT from ESP32 to DAC) | `GPIO_NUM_33` |

These ESP32 GPIO pins are only suggested defaults — the I2S pins are configurable in your code (and some pins may be reserved depending on your board).
