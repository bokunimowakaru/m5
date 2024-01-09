/*******************************************************************************
M5Stack CORE INK にテキスト文字を表示する

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

Ink_Sprite InkPageSprite(&M5.M5Ink);            // M5Ink描画用インスタンス作成
int ink_x = 0;                                  // Ink表示用のX座標
int ink_y = 0;                                  // Ink表示用のY座標
unsigned long ink_start_time = 0;               // 描画完了時刻保持用

void ink_push(){                                // Inkに表示データを転送
    unsigned long t = millis();                 // 現在のCPU時刻を保持
    if(ink_start_time > 0){                     // 初期化or前回描画あり時
        unsigned long t = millis() - ink_start_time; // 前回描画後の経過時間
        if(t < INK_WEIGHT) delay(INK_WEIGHT - t);    // 前回描画後の待機
    }
    ink_start_time = millis();                  // 描画(開始)時刻を保持
    InkPageSprite.pushSprite();                 // push the sprite.
    InkPageSprite.deleteSprite();               // メモリの開放
    InkPageSprite.creatSprite(0,ink_y,200,16,0); // 画像用バッファの再作成
}

void ink_print_setup(){                         // Inkの初期化処理部
    Serial.println("Entered ink_print_setup");  // debug
    while(!M5.M5Ink.isInit()) delay(3000);      // Inkの初期化
    ink_start_time = millis();                  // 初期化処理の完了時刻を保持
    M5.M5Ink.clear();                           // Inkを消去
    ink_x = 0;                                  // Ink表示用のX座標
    ink_y = 0;                                  // Ink表示用のY座標
    InkPageSprite.clear();                      // 2024/1/9 追加
    InkPageSprite.creatSprite(0,0,200,16,0);    // 画像用バッファの作成
    Serial.println("Done");                     // debug
}

void ink_print_clear(){                         // Inkの画面を消去する
    InkPageSprite.deleteSprite();               // メモリの開放
    InkPageSprite.creatSprite(0,0,200,200,0);   // 画面全体
    InkPageSprite.pushSprite();                 // push the sprite.
    while(!M5.M5Ink.isInit()) delay(3000);      // Inkの初期化
    M5.M5Ink.clear();                           // Inkを消去
    InkPageSprite.deleteSprite();               // メモリの開放
    ink_x = 0;                                  // Ink表示用のX座標
    ink_y = 0;                                  // Ink表示用のY座標
    InkPageSprite.creatSprite(0,0,200,16,0);    // 画像用バッファの作成
}

void ink_println(){                             // Inkの改行処理
    ink_x = 0;                                  // (改行処理)X座標を左端へ
    if(ink_y <= 192){                           // 表示可能な場合
        ink_y += 16;                            // (改行処理)Y座標を下の行へ
        ink_push();                             // Inkに表示データを転送
    }
}

void ink_print(String text, bool push = true);
void ink_print(String text, bool push){         // Inkに文字列を表示する
    char c[2];
    Serial.println("TEXT(" + String(text.length()) + ")" + text); // debug
    for(int i=0; i < text.length(); i++){       // 文字数分の繰り返し処理
        text.substring(i).toCharArray(c, 2);    // 1文字+終端の取り出し
        if(c[0] < 0x20 || c[0] >= 0x7f) continue; // 表示不可文字の処理を排除
        InkPageSprite.drawChar(ink_x,0,c[0]);   // バッファに文字を描画
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
