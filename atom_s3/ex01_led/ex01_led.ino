/*******************************************************************************
Example 1: Wi-Fi コンシェルジェ 照明担当 for M5 ATOM S3
・HTTPによるWebサーバ機能搭載 Wi-FiコンシェルジェがLEDを制御します。
・RGB LEDが無くても、電球の画像の切換で動作確認できます。

    使用機材(例)：M5 ATOM S3 + (RGB LED Unit)

                                          Copyright (c) 2021-2023 Wataru KUNINO
*******************************************************************************/

#include "M5AtomS3.h"                           // ATOM S3 用ライブラリ
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WebServer.h>                          // HTTPサーバ用ライブラリ
#include "on_bmp.h"                             // 点灯した電球のJPEGデータ
#include "off_bmp.h"                            // 消灯した電球のJPEGデータ

/******************************************************************************
 Wi-Fi 無線アクセスポイントの設定
 *****************************************************************************/
#define SSID "1234ABCD"                         // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード

/******************************************************************************
 プログラム本体
 *****************************************************************************/
WebServer server(80);                           // Webサーバ(ポート80=HTTP)定義
int led_stat = 0;                               // LED状態用変数led_statを定義

#define PIN_LED_RGB 2                           // RGB LED(Grove互換端子)

void ledControl(boolean on){                    // LED制御関数
    if(on){                                     // on=trueのとき
        led(20);                                // RGB LEDを点灯(輝度20)
        M5.Lcd.drawBitmap(0,0,128,128,on_bmp);  // LCDにJPEG画像onを表示
    }else{                                      // on=falseのとき
        led(0);                                 // RGB LEDを消灯
        M5.Lcd.drawBitmap(0,0,128,128,off_bmp); // LCDにJPEG画像offを表示
    }
}

void handleRoot(){
    String rx, tx;                              // 受信用,送信用文字列
    if(server.hasArg("L")){                     // 引数Lが含まれていた時
        rx = server.arg("L");                   // 引数Lの値を変数rxへ代入
        led_stat = rx.toInt();                  // 変数sの数値をled_statへ
    }
    tx = getHtml(led_stat);                     // HTMLコンテンツを取得
    server.send(200, "text/html", tx);          // HTMLコンテンツを送信
    ledControl(led_stat);                       // LED制御関数ledControlを実行
    String S = "L=" + String(led_stat);         // 表示用変数Sにled_stat値を代入
    M5.Lcd.drawString(S,0,0);                   // LCDに変数Sの内容を表示
}

void btnUpdate(){                               // ボタン状態に応じてLEDを制御
    M5.update();                                // M5Stack用IO状態の更新
    int btn = M5.Btn.wasPressed();              // ボタンの状態をbtnへ代入
    if( btn == 1 ) led_stat = !led_stat;        // ボタン押下時led_statを反転
    if( btnC == 1 ) led_stat = 1;               // ボタンB押下時led_stat=1を代入
    if( btnA || btnC) ledControl(led_stat);     // ボタン操作時にLED制御を実行
}

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    led_setup(PIN_LED_RGB);                     // RGB LED 初期設定(ポート設定)
    ledControl(led_stat);                       // LED制御関数ledControlを実行
//  M5.Lcd.setBrightness(200);                  // LCDの輝度設定(#if 0で未定義)
    M5.Lcd.setRotation(2);                      // LCDを縦向き表示に設定
    M5.Lcd.setTextColor(BLACK);                 // 文字色を黒に
    M5.Lcd.println("ex01 M5 LED HTTP");         // タイトル「LED HTTP」を表示
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイント接続
    while(WiFi.status() != WL_CONNECTED){       // 接続に成功するまで待つ
        led((millis()/50) % 10);                // RGB LEDの点滅
        delay(50);                              // 待ち時間処理
        btnUpdate();                            // ボタン状態を確認
    }
    server.on("/", handleRoot);                 // HTTP接続用コールバック先設定
    server.begin();                             // Web サーバを起動する
    M5.Lcd.println(WiFi.localIP());             // 本機のIPアドレスを表示
}

void loop(){                                    // 繰り返し実行する関数
    server.handleClient();                      // Webサーバの呼び出し
    btnUpdate();                                // ボタン状態を確認
}

/*******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/m5core/arduino

M5Stack Arduino Library API 情報：
https://docs.m5stack.com/en/api/core/system

【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example16_led
https://github.com/bokunimowakaru/esp/tree/master/2_example/example48_led
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex01_led
*******************************************************************************/
