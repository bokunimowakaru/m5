/******************************************************************************
Example 0: Hello, world! for M5 ATOM S3
・起動時にLEDを点滅、LCDにタイトルを表示します。
・本体のボタンを押す/離す/長押しに応じてメッセージを表示します。

                                          Copyright (c) 2019-2023 Wataru KUNINO
*******************************************************************************/

#include "M5AtomS3.h"                           // ATOM S3 用ライブラリ
                                                // 本ライブラリにFastLEDが必要

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    M5.Lcd.fillScreen(BLACK);                   // LCDを消去
//  M5.Lcd.setBrightness(200);                  // LCDの輝度設定(#if 0で未定義)
    M5.Lcd.setRotation(2);                      // LCDを縦向き表示に設定
    M5.Lcd.println("Example 0 M5 ATOM S3");     // LCDにタイトルを表示
}

void loop(){                                    // 繰り返し実行する関数
    M5.Btn.read();                              // ボタンの状態を確認

    int btnA = M5.Btn.wasPressed();             // ボタンが押されたとき
    int btnB = M5.Btn.wasReleased();            // ボタンが離されたとき
    int btnC = M5.Btn.pressedFor(3000);         // ボタンの長押し

    if( btnA == 1 ){                            // ボタンが押された時
        M5.Lcd.println("Hello, world!");        // LCDへメッセージを表示
    }
    if( btnB == 1 ){                            // ボタンが離された時
        M5.Lcd.println("IoT Device M5StickC");  // LCDへメッセージを表示
    }
    if( btnC == 1 ){                            // ボタンの長押し状態で離された時
        M5.Lcd.fillScreen(BLACK);               // LCDを消去
        M5.Lcd.setCursor(0,0);                  // 文字描画位置を画面左上へ
        while(M5.Btn.isPressed())M5.Btn.read(); // ボタンが押されている時は待機
        M5.Lcd.println("Screen Cleared");       // LCDへメッセージを表示
    }
}

/*******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/atoms3/arduino

M5StickC Arduino Library API 情報 (M5StackC 用 ※ATOM S3用ではない )：
https://docs.m5stack.com/en/api/stickc/system_m5stickc

https://github.com/m5stack/M5AtomS3

【引用コード】
https://github.com/bokunimowakaru/m5s/tree/master/m5StickC/example01_hello
*******************************************************************************/
