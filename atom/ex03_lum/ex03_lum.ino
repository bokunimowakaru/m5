/*******************************************************************************
Example 3: ESP32 (IoTセンサ) Wi-Fi 照度計
照度センサ から取得した照度値を送信するIoTセンサです。

使用機材(例)：ATOM + ATOM-HAT(ATOM-MATEに付属) + HAT-DLIGHT

                                          Copyright (c) 2021-2022 Wataru KUNINO
*******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/atom/arduino

ATOM Lite Arduino Library API 情報(本サンプルでは使用しない)：
https://docs.m5stack.com/en/api/atom/system

【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example06_lum
https://github.com/bokunimowakaru/esp/tree/master/2_example/example38_lum
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex03_lum
*******************************************************************************/

#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ
#include "esp_sleep.h"                          // ESP32用Deep Sleep ライブラリ

#define PIN_LED_RGB 27                          // G27 に RGB LED
#define SSID "1234ABCD"                         // 無線LANアクセスポイントSSID
#define PASS "password"                         // パスワード
#define PORT 1024                               // 送信のポート番号
#define SLEEP_P 30*1000000ul                    // スリープ時間 30秒(uint32_t)
#define DEVICE "illum_1,"                       // デバイス名(5字+"_"+番号+",")

/******************************************************************************
 Ambient 設定
 ******************************************************************************
 ※Ambientでのアカウント登録と、チャネルID、ライトキーを取得する必要があります。
    1. https://ambidata.io/ へアクセス
    2. 右上の[ユーザ登録(無料)]ボタンでメールアドレス、パスワードを設定して
       アカウントを登録
    3. [チャネルを作る]ボタンでチャネルIDを新規作成する
    4. 「チャネルID」を下記のAmb_Idのダブルコート(")内に貼り付ける
    5. 「ライトキー」を下記のAmb_Keyに貼り付ける
   (参考文献)
    IoTデータ可視化サービスAmbient(アンビエントデーター社) https://ambidata.io/
*******************************************************************************/
#define Amb_Id  "00000"                         // AmbientのチャネルID
#define Amb_Key "0000000000000000"              // Ambientのライトキー

IPAddress IP_BROAD;                             // ブロードキャストIPアドレス

void setup(){                                   // 起動時に一度だけ実行する関数
    led_setup(PIN_LED_RGB);                     // RGB LEDの初期設定(ポート設定)
    Serial.begin(115200);                       // 動作確認のためのシリアル出力
    Serial.println("M5 LUM");                   // 「M5 LUM」をシリアル出力

    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    while(WiFi.status() != WL_CONNECTED){       // 接続に成功するまで待つ
        led((millis()/50) % 10);                // (RGB LED)LEDの点滅
        if(millis() > 30000) sleep();           // 30秒超過でスリープ
        delay(50);                              // 待ち時間処理
    }
    led(0,20,0);                                // (RGB LED)LEDを緑色で点灯
    IP_BROAD = WiFi.localIP();                  // IPアドレスを取得
    IP_BROAD[3] = 255;                          // ブロードキャストアドレスに
    Serial.println(IP_BROAD);                   // ブロードキャストアドレス表示
    bh1750Setup(19,22);
}

void loop(){                                    // 繰り返し実行する関数
    float lux = getLux();                       // 照度(lux)を取得

    String S = String(DEVICE) + String(lux,0);  // 送信データSにデバイス名を代入
    Serial.println(S);                          // 送信データSをシリアル出力表示
    WiFiUDP udp;                                // UDP通信用のインスタンスを定義
    udp.beginPacket(IP_BROAD, PORT);            // UDP送信先を設定
    udp.println(S);                             // 送信データSをUDP送信
    udp.endPacket();                            // UDP送信の終了(実際に送信する)
    if(strcmp(Amb_Id,"00000") == 0) sleep();    // Ambient未設定時にsleepを実行

    S = "{\"writeKey\":\""+String(Amb_Key);     // (項目)writeKey,(値)ライトキー
    S += "\",\"d1\":\"" + String(lux) + "\"}";  // (項目)d1,(値)照度
    HTTPClient http;                            // HTTPリクエスト用インスタンス
    http.setConnectTimeout(15000);              // タイムアウトを15秒に設定する
    String url = "http://ambidata.io/api/v2/channels/"+String(Amb_Id)+"/data";
    http.begin(url);                            // HTTPリクエスト先を設定する
    http.addHeader("Content-Type","application/json"); // JSON形式を設定する
    Serial.println(url);                        // 送信URLを表示
    http.POST(S);                               // センサ値をAmbientへ送信する
    http.end();                                 // HTTP通信を終了する
    sleep();                                    // 下記のsleep関数を実行
}

void sleep(){                                   // スリープ実行用の関数
    delay(200);                                 // 送信待ち時間
    WiFi.disconnect();                          // Wi-Fiの切断
    led_off();                                  // (RGB LED)LEDの消灯
    Serial.println("Sleep...");                 // 「Sleep」をシリアル出力表示
    esp_deep_sleep(SLEEP_P);                    // Deep Sleepモードへ移行
}
