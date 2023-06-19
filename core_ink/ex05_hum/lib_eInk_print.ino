/*******************************************************************************
M5Stack CORE INK にテキスト文字を表示する

【参考文献】
M5Stack Arduino Library API 情報：
https://docs.m5stack.com/en/api/coreink/system_api

                                               Copyright (c) 2023 Wataru KUNINO
*******************************************************************************/

#include <M5CoreInk.h>                          // M5Stack用ライブラリ組み込み

Ink_Sprite InkPageSprite(&M5.M5Ink);            // M5Inkインスタンス作成
int eInk_x = 0;                                 // E-Ink表示用のX座標
int eInk_y = 0;                                 // E-Ink表示用のY座標
unsigned long eInk_start_time = 0;              // 初期化処理の完了時刻保持用

void eInk_push(){                               // E-Inkに表示データを転送
    if(eInk_start_time > 0){                    // 初回の転送時
        unsigned long t = millis() - eInk_start_time; // 初期化後の経過時間
        if( t < 1000 ) delay(1000 - t);         // 初期化後1000ms経過まで待機
        eInk_start_time = 0;                    // 以上の処理を初回のみにする
    }
    InkPageSprite.pushSprite();                 // push the sprite.
}

void eInk_print_setup(){                        // E-Inkの初期化処理部
    Serial.println("Entered eInk_print_setup"); // debug
    M5.M5Ink.isInit();                          // E-Inkの初期化
    M5.M5Ink.clear();                           // E-Inkを消去
    InkPageSprite.creatSprite(0,0,200,200,0);   // 画像用バッファの作成
//  InkPageSprite.clear(CLEAR_DRAWBUFF|CLEAR_LASTBUFF); // バッファのクリア
    InkPageSprite.pushSprite();                 // push the sprite.
    eInk_x = 0;                                 // E-Ink表示用のX座標
    eInk_y = 0;                                 // E-Ink表示用のY座標
    eInk_start_time = millis();                 // 初期化処理の完了時刻を保持
    Serial.println("Done");                     // debug
}

void eInk_println(bool push=true);
void eInk_println(bool push){                   // E-Inkの改行処理
    eInk_x = 0;                                 // (改行処理)X座標を左端へ
    eInk_y += 16;                               // (改行処理)Y座標を下の行へ
    if(eInk_y >= 200) eInk_y = 200 - 16;        // 最下段を超えた時に最下段へ
    if(push) eInk_push();                       // E-Inkに表示データを転送
}

void eInk_print(String text, bool push){        // E-Inkに文字列を表示する
    char c[2];
    Serial.println("TEXT(" + String(text.length()) + ")" + text); // debug
    for(int i=0; i < text.length(); i++){       // 文字数分の繰り返し処理
        text.substring(i).toCharArray(c, 2);    // 1文字+終端の取り出し
        if(c[0] < 0x20 || c[0] >= 0x7f) continue;   // 表示不可文字の処理を排除
        InkPageSprite.drawChar(eInk_x,eInk_y,c[0]); // バッファに文字を描画
        eInk_x += 8;                            // 座標1文字分
        if(eInk_x >= 200){                      // X座標が右端を超えた時
            eInk_println(false);                // 改行処理
        }
    }
    if(push) eInk_push();                       // E-Inkに表示データを転送
}
void eInk_print(String text){eInk_print(text, true);}

void eInk_println(uint32_t ip, bool push){      // IPアドレスを表示する
    Serial.println("Entered println ip");       // debug
    char s[16];
    sprintf(s,"%d.%d.%d.%d",ip&255,(ip>>8)&255,(ip>>16)&255,(ip>>24)&255);
    eInk_print(String(s),false);                // eInk_printで文字列を表示
    eInk_println(push);                         // 改行
}
void eInk_println(uint32_t ip){ eInk_println(ip, true);}

void eInk_println(String text, bool push){      // E-Inkに文字列を表示する
    Serial.println("Entered println text");     // debug
    eInk_print(text, false);                    // eInk_printで文字列を表示
    eInk_println(push);                         // 改行
}
void eInk_println(String text){ eInk_println(text, true);}

void eInk_print(const char *text, bool push){ eInk_print(String(text),push);}
void eInk_print(const char *text){ eInk_print(String(text),true);}
void eInk_println(const char *text, bool push){ eInk_println(String(text),push);}
void eInk_println(const char *text){ eInk_println(String(text),true); }

/******************************************************************************
【参考文献】 上記は、下記のコードを参考に国野亘が作成しました。
*******************************************************************************
* Copyright (c) 2021 by M5Stack
*                  Equipped with CoreInk sample source code
*                          配套  CoreInk 示例源代码
* Visit the website for more information：https://docs.m5stack.com/en/core/coreink
* 获取更多资料请访问：https://docs.m5stack.com/zh_CN/core/coreink
*
* describe：Hello World
* date：2021/11/14
*******************************************************************************

#include "M5CoreInk.h"

Ink_Sprite InkPageSprite(&M5.M5Ink);  //创建 M5Ink实例

  After CoreInk is started or reset
  the program in the setUp () function will be run, and this part will only be run once.
  在 CoreInk 启动或者复位后，即会开始执行setup()函数中的程序，该部分只会执行一次。
  
void setup() {
  M5.begin(); //Initialize CoreInk. 初始化 CoreInk
  if( !M5.M5Ink.isInit()){  //check if the initialization is successful. 检查初始化是否成功
    Serial.printf("Ink Init faild");
    while (1) delay(100);
  }
  M5.M5Ink.clear(); //clear the screen. 清屏
  delay(1000);
  //creat ink Sprite. 创建图像区域
  if( InkPageSprite.creatSprite(0,0,200,200,true) != 0 ){
    Serial.printf("Ink Sprite creat faild");
  }
  InkPageSprite.drawString(35,50,"Hello World!"); //draw string. 绘制字符串
  InkPageSprite.pushSprite();  //push the sprite.  推送图片
}

void loop() {

}
*/
