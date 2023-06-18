/*******************************************************************************
M5Stack CORE INK にテキスト文字を表示する

【参考文献】
M5Stack Arduino Library API 情報：
https://docs.m5stack.com/en/api/coreink/system_api

                                               Copyright (c) 2016 Wataru KUNINO
*******************************************************************************/

#include <M5CoreInk.h>                          // M5Stack用ライブラリ組み込み

Ink_Sprite InkPageSprite(&M5.M5Ink);            // M5Inkインスタンス作成
int eInk_x = 0;                                 // E-Ink表示用のX座標
int eInk_y = 0;                                 // E-Ink表示用のY座標

void eInk_print_setup(){
    M5.M5Ink.isInit();                          // E-Inkの初期化
    M5.M5Ink.clear();                           // E-Inkを消去
}

void eInk_println(){                            // E-Inkの改行処理
    eInk_x = 0;                                 // (改行処理)X座標を左端へ
    eInk_y += 16;                               // (改行処理)Y座標を下の行へ
    if(eInk_y >= 200) eInk_y = 200 - 16;        // 最下段を超えた時に最下段へ
}

void eInk_print(String text){                   // E-Inkに文字列を表示する
    char c[2];
    InkPageSprite.creatSprite(0,0,200,200,0);   // 画像用バッファの作成
    for(int i=0; i < text.length(); i++){       // 文字数分の繰り返し処理
        text.substring(i).toCharArray(c, 2);    // 1文字+終端の取り出し
        if(c[0] < 0x20 || c[0] >= 0x7f) continue;   // 表示不可文字の処理を排除
        InkPageSprite.drawChar(eInk_x,eInk_y,c[0]); // バッファに文字を描画
        eInk_x += 8;                            // 座標1文字分
        if(eInk_x >= 200){                      // X座標が右端を超えた時
            eInk_println();                     // 改行処理
        }
    }
    InkPageSprite.pushSprite();                 // push the sprite.
}

void eInk_println(uint32_t ip){                 // IPアドレスを表示する
    char s[16];
    sprintf(s,"%d.%d.%d.%d",ip&255,(ip>>8)&255,(ip>>16)&255,(ip>>24)&255);
    eInk_print(String(s));                      // eInk_printで文字列を表示
    eInk_println();                             // 改行
}


void eInk_println(String text){                 // E-Inkに文字列を表示する
    eInk_print(text);                           // eInk_printで文字列を表示
    eInk_x = 0;                                 // (改行処理)X座標を左端へ
    eInk_y += 16;                               // (改行処理)Y座標を下の行へ
    if(eInk_y >= 200) eInk_y = 200 - 16;        // 最下段を超えた時に最下段へ
}
