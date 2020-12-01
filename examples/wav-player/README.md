# ESP WAV player example

Connect to ESP8266 I2S DAC like PCM5102 with pinout as below:

| pin name| function | gpio_num |
|:---:|:---:|:---:|
| LRCK |word select| GPIO_NUM_2 |
| SCK  |continuous serial clock| GPIO_NUM_15 |
| DIN  |serial data| GPIO_NUM_3 |
