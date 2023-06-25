/*******************************************************************************
M5Stack CORE INK にテキスト文字を表示する [バッファへの描画のみ]

対応e-Paper：
1.54 inch E-paper Display Series GDEW0154M09, Dalian Good Display Co., Ltd

                                               Copyright (c) 2023 Wataru KUNINO
********************************************************************************
本ソースコードの作成にあたり、下記の文献を参考にしました。

【参考文献】
M5Stack Arduino Library API 情報：
https://docs.m5stack.com/en/api/coreink/system_api
https://docs.m5stack.com/en/api/coreink/eink_api
*******************************************************************************/

#include <M5CoreInk.h>                          // M5Stack用ライブラリ組み込み
#define INK_WEIGHT 240                          // 再描画までの間隔(ms)
                                                // 範囲240～15000 大きい方が遅い
Ink_Sprite *InkTextSprite;                      // M5Ink描画用ポインタ

int ink_x = 0;                                  // Ink表示用のX座標
int ink_y = 0;                                  // Ink表示用のY座標

void ink_push(){                                // Inkに表示データを転送
    InkTextSprite->pushSprite();                // push the sprite.
}

void ink_print_setup(Ink_Sprite *sprite, int y=0);
void ink_print_setup(Ink_Sprite *sprite, int y){ // Inkの初期化処理部
    Serial.println("Entered ink_print_setup");  // debug
    InkTextSprite = sprite;
    ink_x = 0;                                  // Ink表示用のX座標
    ink_y = y;                                  // Ink表示用のY座標
    Serial.println("Done");                     // debug
}


void ink_println(){                             // Inkの改行処理
    ink_x = 0;                                  // (改行処理)X座標を左端へ
    if(ink_y <= 192){                           // 表示可能な場合
        ink_y += 16;                            // (改行処理)Y座標を下の行へ
    }
    ink_push();                                 // Inkに表示データを転送
}

void ink_printPos(int y){
    if(y%16) ink_y = y + 16 - (y%16);
    else ink_y = y;
}

void ink_print(String text, bool push = true);
void ink_print(String text, bool push){         // Inkに文字列を表示する
    char c[2];
    Serial.println("TEXT(" + String(text.length()) + ")" + text); // debug
    for(int i=0; i < text.length(); i++){       // 文字数分の繰り返し処理
        text.substring(i).toCharArray(c, 2);    // 1文字+終端の取り出し
        if(c[0] < 0x20 || c[0] >= 0x7f) continue; // 表示不可文字の処理を排除
        InkTextSprite->drawChar(ink_x,ink_y,c[0]);   // バッファに文字を描画
        ink_x += 8;                             // 座標1文字分
        if(ink_x >= 200){                       // X座標が右端を超えた時
            ink_println();                      // 改行処理
        }
    }
    if(push) ink_push();                        // Inkに表示データを転送
}

void ink_print(const char *text){
    ink_print(String(text));
}

void ink_print(const char *text, bool push){
    ink_print(String(text), push);
}

void ink_println(uint32_t ip){                  // IPアドレスを表示する
    char s[16];
    sprintf(s,"%d.%d.%d.%d",ip&255,(ip>>8)&255,(ip>>16)&255,(ip>>24)&255);
    ink_print(String(s), false);                // ink_printで文字列を表示
    ink_println();                              // 改行
}

void ink_println(String text){                  // Inkに文字列を表示する
    ink_print(text, false);                     // ink_printで文字列を表示
    ink_println();                              // 改行
}

void ink_println(const char *text){
    ink_println(String(text));
}

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

Ink_Sprite InkTextSprite(&M5.M5Ink);  //创建 M5Ink实例

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
  if( InkTextSprite->creatSprite(0,0,200,200,true) != 0 ){
    Serial.printf("Ink Sprite creat faild");
  }
  InkTextSprite->drawString(35,50,"Hello World!"); //draw string. 绘制字符串
  InkTextSprite->pushSprite();  //push the sprite.  推送图片
}

void loop() {

}
*/
