/******************************************************************************
Example 0: Hello, world! for ESP32 / ATOM / ATOM Lite
・起動時にLEDを点滅、モールス信号でタイトルを表示します。
・本体のボタンを押すと、メッセージをモールス出力します。

                                          Copyright (c) 2019-2022 Wataru KUNINO
*******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/atom/arduino

ATOM Lite Arduino Library API 情報：
https://docs.m5stack.com/en/api/atom/system

【引用コード】
https://github.com/bokunimowakaru/m5s/tree/master/example01_hello
*******************************************************************************/

// #include <M5Atom.h>                          // M5 ATOM Lite用ライブラリ
#define PIN_LED_RGB 27                          // G27 に RGB LED
#define PIN_BTN_A 39                            // G39 に 操作ボタン

void setup(){                                   // 起動時に一度だけ実行する関数
    pinMode(PIN_BTN_A, INPUT_PULLUP);           // ボタン入力ポート
    led_setup(PIN_LED_RGB);                     // LEDの初期設定(ポートを設定)
    Serial.begin(115200);                       // 動作確認のためのシリアル出力
    led(20);                                    // LED ON (輝度20)
    delay(500);                                 // 100msの待ち時間処理
    led(0);                                     // LED OFF
    delay(500);                                 // 100msの待ち時間処理

//  M5.begin();                                 // M5Stack用ライブラリの起動
    Serial.println("Example 0 ATOM");           // LCDにタイトルを表示
    morse(-1,50,"EXAMPLE 0");                   // モールス符号(-1,速度50,文字)
}

void loop(){                                    // 繰り返し実行する関数
//  M5.BtnA.read();                             // ボタンAの状態を確認
//  M5.BtnB.read();                             // ボタンBの状態を確認
//  int btnA = M5.BtnA.wasPressed();            // ボタンAの状態をbtnAへ代入
//  int btnB = M5.BtnB.wasPressed();            // ボタンBの状態をbtnBへ代入

    int btnA = !digitalRead(PIN_BTN_A);

    if( btnA == 1 ){                            // ボタンAが押されていた時
        delay(300);                             // 300msの待機時間
        if(digitalRead(PIN_BTN_A)){             // 通常の操作
            Serial.println("Hello, world!");    // メッセージをシリアル出力
            morse(-1,50,"HELLO WORLD");         // メッセージをモールス出力
        }else{                                  // 長押し
            Serial.println("IoT Device M5");    // メッセージをシリアル出力
            morse(-1,50,"IOT DEVICE M5");       // メッセージをモールス出力
        }
    }
}
