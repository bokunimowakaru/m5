/*******************************************************************************
Example 6: ESP32 (IoTセンサ) 【Wi-Fi 人感センサ子機】ディープスリープ版
・人感センサ PIR Unit が人体などの動きを検知すると、ディープスリープから復帰し、
　UDPブロードキャストでセンサ値を送信します。

    M5Stack ATOM / ATOM Lite + ATOM-HAT(ATOM-MATEに付属) + PIR HAT(AS312) に対応

                                           Copyright (c) 2016-2022 Wataru KUNINO
********************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/atom/arduino

ATOM Lite Arduino Library API 情報(本サンプルでは使用しない)：
https://docs.m5stack.com/en/api/atom/system

SB412A データシート (NANYANG SENBA OPTICAL AND ELECTRONIC CO. LTD.)

BS612 AS612 データシート (NANYANG SENBA OPTICAL AND ELECTRONIC CO. LTD.)

【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example11_pir
https://github.com/bokunimowakaru/esp/tree/master/2_example/example43_pir
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex06_pir
*******************************************************************************/

#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ
#include "esp_sleep.h"                          // ESP32用Deep Sleep ライブラリ

/******************************************************************************
 LINE Notify 設定
 ******************************************************************************
 ※LINE アカウントと LINE Notify 用のトークンが必要です。
    1. https://notify-bot.line.me/ へアクセス
    2. 右上のアカウントメニューから「マイページ」を選択
    3. トークン名「esp32」を入力
    4. 送信先のトークルームを選択する(「1:1でLINE Notifyから通知を受け取る」等)
    5. [発行する]ボタンでトークンが発行される
    6. [コピー]ボタンでクリップボードへコピー
    7. 下記のLINE_TOKENのダブルコート(")内に貼り付け
 *****************************************************************************/
#define LINE_TOKEN  "your_token"                // LINE Notify トークン★要設定

#define PIN_LED_RGB 27                          // G27 に RGB LED
#define PIN_PIR 33                              // G33にセンサ(人感/ドア)を接続
#define PIN_PIR_GPIO_NUM GPIO_NUM_33            // G33 をスリープ解除信号へ設定
#define SSID "1234ABCD"                         // 無線LANアクセスポイントSSID
#define PASS "password"                         // パスワード
#define PORT 1024                               // 受信ポート番号
#define DEVICE "pir_s_5,"                       // 人感センサ時デバイス名
#define PIR_XOR 0                               // センサ送信値の論理反転の有無

/******************************************************************************
 UDP 宛先 IP アドレス設定
 ******************************************************************************
 カンマ区切りでUPD宛先IPアドレスを設定してください。
 末尾を255にすると接続ネットワーク(アクセスポイント)にブロードキャスト
 *****************************************************************************/
IPAddress UDPTO_IP = {255,255,255,255};         // UDP宛先 IPアドレス

RTC_DATA_ATTR boolean PIR;                      // pir値のバックアップ保存用
boolean pir;                                    // 人感センサ値orドアセンサ状態
int wake = (int)esp_sleep_get_wakeup_cause();   // 起動理由を変数wakeに保存

void setup(){                                   // 起動時に一度だけ実行する関数
    pinMode(PIN_PIR,INPUT);                     // センサ接続したポートを入力に
    pir = digitalRead(PIN_PIR);                 // 人感センサの状態を取得
    led_setup(PIN_LED_RGB);                     // RGB LEDの初期設定(ポート設定)

    Serial.begin(115200);                       // 動作確認のためのシリアル出力
    Serial.println("M5 PIR/Reed");              // 「M5 PIR/Reed」を出力
    Serial.print(" Wake = ");                   // 「wake =」をシリアル出力表示
    Serial.println(wake);                       // 起動理由noをシリアル出力表示
    if(wake != ESP_SLEEP_WAKEUP_EXT0) sleep();  // ボタン以外で起動時にスリープ

    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    while(WiFi.status() != WL_CONNECTED){       // 接続に成功するまで待つ
        led((millis()/50) % 10);                // RGB LEDの点滅
        if(millis() > 30000) sleep();           // 30秒超過でスリープ
        delay(50);                              // 待ち時間処理
    }
    led(0,20,0);                                // RGB LEDを緑色で点灯
    Serial.print(WiFi.localIP());               // 本機のアドレスをシリアル出力
    Serial.print(" -> ");                       // 矢印をシリアル出力
    Serial.println(UDPTO_IP);                   // UDPの宛先IPアドレスを出力
}

void loop(){                                    // 繰り返し実行する関数
    pir = digitalRead(PIN_PIR);                 // 人感センサの最新の状態を取得
    String S = String(DEVICE);                  // 送信データ保持用の文字列変数
    S += String(int(PIR ^ PIR_XOR)) + ", ";     // 起動時PIR値を送信データに追記
    S += String(int(pir ^ PIR_XOR));            // 現在のpir値を送信データに追記
    Serial.println(S);                          // シリアル出力表示

    WiFiUDP udp;                                // UDP通信用のインスタンスを定義
    udp.beginPacket(UDPTO_IP, PORT);            // UDP送信先を設定
    udp.println(S);                             // センサ値を送信
    udp.endPacket();                            // UDP送信の終了(実際に送信する)
    delay(10);                                  // 送信待ち時間

    HTTPClient http;                            // HTTPリクエスト用インスタンス
    http.setConnectTimeout(15000);              // タイムアウトを15秒に設定する
    String url;                                 // URLを格納する文字列変数を生成
    if(strlen(LINE_TOKEN) > 42){                // LINE_TOKEN設定時
        url = "https://notify-api.line.me/api/notify";  // LINEのURLを代入
        Serial.println(url);                    // 送信URLを表示
        http.begin(url);                        // HTTPリクエスト先を設定する
        http.addHeader("Content-Type","application/x-www-form-urlencoded");
        http.addHeader("Authorization","Bearer " + String(LINE_TOKEN));
        http.POST("message=センサが反応しました。(" + S.substring(8) + ")");
        http.end();                             // HTTP通信を終了する
    }
    sleep();                                    // 下記のsleep関数を実行
}

void sleep(){                                   // スリープ実行用の関数
    delay(100);                                 // 送信完了の待ち時間処理
    WiFi.disconnect();                          // Wi-Fiの切断
    int i = 0;                                  // ループ用の数値変数i
    while(i<100){                               // スイッチ・ボタン解除待ち
        boolean pir_b = digitalRead(PIN_PIR);   // 現在の値をpir_bに保存
        if( pir == pir_b) i++;                  // 値に変化がない時はiに1を加算
        else{                                   // 変化があった時
            i = 0;                              // 繰り返し用変数iを0に
            pir = pir_b;                        // pir値を最新の値に更新
        }
        delay(1);                               // 待ち時間処理
    }
    PIR = !pir;                                 // sleep時の保存可能な変数に保持
    boolean pir_wake = 0;
    if(PIR) pir_wake = 1;                       // 次回、IOがHighのときに起動
    led_off();                                  // RGB LEDの消灯
    Serial.println("Sleep...");                 // 「Sleep」をシリアル出力表示
    esp_sleep_enable_ext0_wakeup(PIN_PIR_GPIO_NUM, pir_wake);   // 割込み設定
    esp_deep_sleep_start();                     // Deep Sleepモードへ移行
}
