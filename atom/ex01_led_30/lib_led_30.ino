/***********************************************************************
LED制御ドライバ RGB LED WS2812
                                        Copyright (c) 2022 Wataru KUNINO
************************************************************************
MITライセンスで配布します。権利表示の改変は禁止します。全て無保証です。
本ソースコードには末尾に示すライセンス(The Unlicense)に基づいたコードを
含みます。
***********************************************************************/

#include "driver/rmt.h"

// Configure these based on your project needs using menuconfig ********
#define LED_RMT_TX_CHANNEL  (rmt_channel_t)0
int _PIN_LED = 8;

#define NUM_LEDS 30
#define BITS_PER_LED_CMD 24
#define LED_BUFFER_ITEMS ((NUM_LEDS * BITS_PER_LED_CMD))

struct led_state {
    uint32_t leds[NUM_LEDS];
};

struct led_state buf_state;

// These values are determined by measuring pulse timing
#define T0H 14  // the signal should be high when a 0 bit is transmitted
#define T1H 52  // the signal should be high when a 1 bit is transmitted
#define TL  52  // should be low followed by the high time for either

// This is the buffer which the hw peripheral pulsing the output pin
rmt_item32_t led_data_buffer[LED_BUFFER_ITEMS];

void setup_rmt_data_buffer(struct led_state new_state);

void ws2812_control_init(void){
    rmt_config_t config;
    config.rmt_mode = RMT_MODE_TX;
    config.channel = LED_RMT_TX_CHANNEL;
    config.gpio_num = (gpio_num_t)_PIN_LED;
    config.mem_block_num = 3;
    // glancek commented on 30 Mar 2021; not work without flag = 0
    config.flags = 0;
    config.tx_config.loop_en = false;
    config.tx_config.carrier_en = false;
    config.tx_config.idle_output_en = true;
    config.tx_config.idle_level = (rmt_idle_level_t)0;
    config.clk_div = 2;
    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
    memset(&buf_state,0,sizeof(buf_state));
}

void ws2812_write_leds(struct led_state new_state){
    setup_rmt_data_buffer(new_state);
    ESP_ERROR_CHECK(rmt_write_items(
        LED_RMT_TX_CHANNEL, led_data_buffer, LED_BUFFER_ITEMS, false
    ));
    ESP_ERROR_CHECK(rmt_wait_tx_done(
        LED_RMT_TX_CHANNEL, portMAX_DELAY
    ));
    memcpy(&buf_state,&new_state,sizeof(buf_state));
}

void setup_rmt_data_buffer(struct led_state new_state){
    for (uint32_t led = 0; led < NUM_LEDS; led++) {
        uint32_t bits_to_send = new_state.leds[led];
        uint32_t mask = 1 << (BITS_PER_LED_CMD - 1);
        for (uint32_t bit = 0; bit < BITS_PER_LED_CMD; bit++) {
            uint32_t bit_is_set = bits_to_send & mask;
            led_data_buffer[led * BITS_PER_LED_CMD + bit]
                = bit_is_set ?
                    (rmt_item32_t){{{T1H, 1, TL, 0}}} :
                    (rmt_item32_t){{{T0H, 1, TL, 0}}};
            mask >>= 1;
        }
    }
}

/* 引数r,g,bに代入された色をLEDに送信する。値は0～255の範囲で設定 */
void led(int r,int g,int b){                    // LEDにカラーを設定
    uint32_t rgb = (g & 0xff) << 16 | (r & 0xff) << 8 | (b & 0xff);
    struct led_state new_state;
    for(int i=0;i<NUM_LEDS;i++){
        new_state.leds[i] = rgb;
    }
    ws2812_write_leds(new_state);
}

void led_single(int r,int g,int b){                    // LEDにカラーを設定
    uint32_t rgb = (g & 0xff) << 16 | (r & 0xff) << 8 | (b & 0xff);
    struct led_state new_state;
    new_state.leds[0] = rgb;
    for(int i=1;i<NUM_LEDS;i++){
        new_state.leds[i] = buf_state.leds[i-1];
    }
    ws2812_write_leds(new_state);
}

/* 000～30までをLEDの点灯数で表示 */
void led_bar(int n, uint32_t rgb){
    struct led_state new_state;
    for(int i=0;i<NUM_LEDS;i++){
        if(i+1 <= n) new_state.leds[i] = rgb;
        else new_state.leds[i] = 0;
    }
    ws2812_write_leds(new_state);
}

void led_bar(int n){
    led_bar(n, 10 << 16 | 10 << 8 | 10);
}

/* 000～999までの数字をLEDの点灯数で表示 */
void led_num(int n, uint32_t rgb){
    struct led_state new_state;
    for(int i=0;i<NUM_LEDS;i++){
        int j = i%10;
        if(j == 0) new_state.leds[i] = 10 << 16;
        else if(j <= n%10) new_state.leds[i] = rgb;
        else new_state.leds[i] = 0;
        if(j == 9) n /= 10;
    }
    ws2812_write_leds(new_state);
}

void led_num(int n){
    led_num(n, 10 << 16 | 10 << 8 | 10);
}


void led(int brightness){                       // グレースケール制御
//  if(brightness > 0xff) brightness = 0xff;    // 256以上時に255に設定
    if(brightness > 100) brightness = 100;      // 100以上時に100に設定(電流上限)
    led(brightness,brightness,brightness);      // RGB全て同値でLED制御
}

void led_single(int brightness){
    if(brightness > 100) brightness = 100;      // 100以上時に100に設定(電流上限)
    led_single(brightness,brightness,brightness); // RGB全て同値でLED制御
}

void led_on(){                                  // LED制御の停止
    led(30);                                    // LEDの点灯
}
void led_on_single(){                           // LED制御の停止
    led_single(30);                             // LEDの点灯
}

void led_off(){                                 // LED制御の停止
    led(0);                                     // LEDの消灯
}
void led_off_single(){                          // LED制御の停止
    led_single(0);                              // LEDの消灯
}

void led_setup(int pin){
    _PIN_LED = pin;
    ws2812_control_init();
    led_off();
}

void led_setup(){
    led_setup(_PIN_LED);
}

void led_tone2rgb(byte rgb[], int tone_color, int brightness){
    float r, g, b, v, f, q, t;
    v = (float)brightness / 255.;
    f = ((float)(tone_color % 60)) / 60.;
    t = f * v;
    q = v - t;
    switch(tone_color / 60){
        case 0: r=v; g=t; b=0; break;
        case 1: r=q; g=v; b=0; break;
        case 2: r=0; g=v; b=t; break;
        case 3: r=0; g=q; b=v; break;
        case 4: r=t; g=0; b=v; break;
        case 5: r=v; g=0; b=q; break;
        default: r=v; g=t; b=0; break;
    }
    rgb[0] = (byte)(r * 255. + .5);
    rgb[1] = (byte)(g * 255. + .5);
    rgb[2] = (byte)(b * 255. + .5);
}

void led_notify(int mode){
	int tone_color = 0;                             // 現在の色調(0〜359)
	int tone_speed = 3;                             // 色調の変更速度
	int brightness = 0;                             // 現在の輝度値
	int dimmer_speed = +1;                          // 輝度の変更速度
	int dimmer_max = 14;                            // 点灯時の輝度(255以下)
    byte rgb[3];
    
    struct led_state buf2;
    memcpy(&buf2, &buf_state, sizeof(buf2));

    if(mode==1){
	    while(tone_color >= 0 && tone_color < 360){
		    led_tone2rgb(rgb, tone_color, brightness);
		    led_single(rgb[0],rgb[1],rgb[2]);           // LED 制御
		    brightness += dimmer_speed;                 // 輝度の増減
		    tone_color += tone_speed;
		    if(brightness < 0){                         // 輝度値が負になったとき
		        brightness = 0;                         // 輝度値を0に設定
		        dimmer_speed = abs(dimmer_speed);       // 正の速度を設定
		    }else if(brightness > dimmer_max){          // 点灯時の輝度を超えたとき
		        brightness = dimmer_max;                // 転倒時の輝度を設定
		        dimmer_speed = -abs(dimmer_speed);      // 負の速度
		    }
	    	delay(14);                                  // 14msの待ち時間処理
		}
	}else if(mode==2){
	    while(tone_color >= 0 && tone_color < 360){
		    led_tone2rgb(rgb, tone_color, brightness);
		    led(rgb[0],rgb[1],rgb[2]);                  // LED 制御
		    brightness += dimmer_speed;                 // 輝度の増減
		    tone_color += tone_speed;
		    if(brightness < 0){                         // 輝度値が負になったとき
		        brightness = 0;                         // 輝度値を0に設定
		        dimmer_speed = abs(dimmer_speed);       // 正の速度を設定
		    }else if(brightness > dimmer_max){          // 点灯時の輝度を超えたとき
		        brightness = dimmer_max;                // 転倒時の輝度を設定
		        dimmer_speed = -abs(dimmer_speed);      // 負の速度
		    }
	    	delay(14);                                  // 14msの待ち時間処理
		}
	}
    ws2812_write_leds(buf2);
}


/******************************************************************************
lib_led.ino
*******************************************************************************
MIT License

Copyright (c) 2022 Wataru KUNINO

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

    /**********************************************************************
    参考文献：
    https://github.com/JSchaenzle/ESP32-NeoPixel-WS2812-RMT
    ***********************************************************************

    2018年10月4日 時点のライセンス：

    JSchaenzle/ESP32-NeoPixel-WS2812-RMT is licensed under the

    The Unlicense

    A license with no conditions whatsoever which dedicates works to the
    public domain. Unlicensed works, modifications, and larger works may be
    distributed under different terms and without source code.

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or
    distribute this software, either in source code form or as a compiled
    binary, for any purpose, commercial or non-commercial, and by any
    means.

    In jurisdictions that recognize copyright laws, the author or authors
    of this software dedicate any and all copyright interest in the
    software to the public domain. We make this dedication for the benefit
    of the public at large and to the detriment of our heirs and
    successors. We intend this dedication to be an overt act of
    relinquishment in perpetuity of all present and future rights to this
    software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
    OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.

    For more information, please refer to <http://unlicense.org>
    ***********************************************************************/
