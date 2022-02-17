/******************************************************************************
Example 01: Hello, world! for M5StickC PLUS
・起動時にLEDを点滅、LCDにタイトルを表示します。
・本体のボタンを押すと、ボタンに応じてメッセージを表示します。

                                          Copyright (c) 2019-2022 Wataru KUNINO
*******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/m5stickc_plus/arduino

M5StickC Arduino Library API 情報 (旧モデル M5StackC 用)：
https://docs.m5stack.com/en/api/stickc/system_m5stickc
*******************************************************************************/

#include <M5StickCPlus.h>                       // M5StickC Plus 用ライブラリ

void setup(){                                   // 起動時に一度だけ実行する関数
    pinMode(M5_LED,OUTPUT);                     // LEDのIOを出力に設定
    digitalWrite(M5_LED,LOW);                   // LED ON
    delay(100);                                 // 100msの待ち時間処理
    digitalWrite(M5_LED,HIGH);                  // LED OFF

    M5.begin();                                 // M5Stack用ライブラリの起動
    M5.Lcd.fillScreen(BLACK);                   // LCDを消去
    M5.Axp.ScreenBreath(7+2);                   // LCDの輝度を2に設定
    M5.Lcd.setRotation(1);                      // LCDを横向き表示に設定
    M5.Lcd.println("Example 0 M5StickC LCD");   // LCDにタイトルを表示
}

void loop(){                                    // 繰り返し実行する関数
    M5.BtnA.read();                             // ボタンAの状態を確認
    M5.BtnB.read();                             // ボタンBの状態を確認

    int btnA = M5.BtnA.wasPressed();            // ボタンAの状態をbtnAへ代入
    int btnB = M5.BtnB.wasPressed();            // ボタンBの状態をbtnBへ代入

    if( btnA == 1 ){                            // ボタンAが押されていた時
        M5.Lcd.println("Hello, world!");        // LCDへメッセージを表示
    }
    if( btnB == 1 ){                            // ボタンBが押されていた時
        M5.Lcd.println("IoT Device M5StickC");  // LCDへメッセージを表示
    }
    if( M5.Axp.GetBtnPress() ){                 // 電源が押された時
        M5.Lcd.fillScreen(BLACK);               // LCDを消去
        M5.Lcd.setCursor(0,0);                  // 文字描画位置を画面左上へ
        M5.Lcd.println("Screen Cleared");       // LCDへメッセージを表示
    }
}
