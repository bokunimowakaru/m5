/*******************************************************************************
Example 5: ESP32 (IoTセンサ) Wi-Fi 温湿度計 SENSIRION製 SHT30/SHT31/SHT35 版
・デジタルI2Cインタフェース搭載センサから取得した温湿度を送信するIoTセンサです。

    使用機材(例)：ATOM/ATOM Lite  + ATOM-HAT(ATOM-MATEに付属) + ENV II/III HAT

    ESP32 のI2Cポート:
        SHT30/SHT31/SHT35 SDAポート G19
        SHT30/SHT31/SHT35 SCLポート G22    設定方法＝shtSetup(SDA,SCL)

注意: ENV HATのバージョンによって搭載されているセンサが異なります。
      このプログラムは SHT30 用です。ENV HAT には対応していません。

ENV HAT     DHT12 + BMP280 + BMM150
ENV II HAT  SHT30 + BMP280 + BMM150
ENV III HAT SHT30 + QMP6988
*******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/atom/arduino

ATOM Lite Arduino Library API 情報(本サンプルでは使用しない)：
https://docs.m5stack.com/en/api/atom/system

【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example09_hum_sht31
https://github.com/bokunimowakaru/esp/tree/master/2_example/example41_hum_sht31
https://github.com/bokunimowakaru/m5s/tree/master/example04d_temp_hum_sht
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex05_hum

                                          Copyright (c) 2016-2022 Wataru KUNINO
*******************************************************************************/

#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ
#include "esp_sleep.h"                          // ESP32用Deep Sleep ライブラリ

#define PIN_LED_RGB 27                          // G27 に RGB LED
#define SSID "1234ABCD"                         // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
#define PORT 1024                               // 送信のポート番号
#define SLEEP_P 30*1000000ul                    // スリープ時間 30秒(uint32_t)
#define DEVICE "humid_5,"                       // デバイス名(5字+"_"+番号+",")

/******************************************************************************
 Ambient 設定
 ******************************************************************************
 ※Ambientのアカウント登録と、チャネルID、ライトキーの取得が必要です。

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

/******************************************************************************
 UDP 宛先 IP アドレス設定
 ******************************************************************************
 カンマ区切りでUPD宛先IPアドレスを設定してください。
 末尾を255にすると接続ネットワーク(アクセスポイント)にブロードキャスト
 *****************************************************************************/
IPAddress UDPTO_IP = {255,255,255,255};         // UDP宛先 IPアドレス

void setup(){                                   // 起動時に一度だけ実行する関数
    led_setup(PIN_LED_RGB);                     // RGB LEDの初期設定(ポート設定)
    shtSetup(19,22);                            // 湿度センサの初期化
    Serial.begin(115200);                       // 動作確認のためのシリアル出力
    Serial.println("M5 HUM");                   // 「M5 HUM」をシリアル出力

    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    while(WiFi.status() != WL_CONNECTED){       // 接続に成功するまで待つ
        led((millis()/50) % 10);                // RGB LED の点滅
        if(millis() > 30000) sleep();           // 30秒超過でスリープ
        delay(50);                              // 待ち時間処理
    }
    led(0,20,0);                                // RGB LED を緑色で点灯
    Serial.print(WiFi.localIP());               // 本機のアドレスをシリアル出力
    Serial.print(" -> ");                       // 矢印をシリアル出力
    Serial.println(UDPTO_IP);                   // UDPの宛先IPアドレスを出力
}

void loop(){                                    // 繰り返し実行する関数
    float temp = getTemp();                     // 温度を取得して変数tempに代入
    float hum =getHum();                        // 湿度を取得して変数humに代入
    if(temp < -100. || hum < 0.) sleep();       // 取得失敗時に末尾のsleepを実行

    String S = String(DEVICE);                  // 送信データSにデバイス名を代入
    S += String(temp,1) + ", ";                 // 変数tempの値を追記
    S += String(hum,1);                         // 変数humの値を追記
    Serial.println(S);                          // 送信データSをシリアル出力表示
    WiFiUDP udp;                                // UDP通信用のインスタンスを定義
    udp.beginPacket(UDPTO_IP, PORT);            // UDP送信先を設定
    udp.println(S);                             // 送信データSをUDP送信
    udp.endPacket();                            // UDP送信の終了(実際に送信する)
    if(strcmp(Amb_Id,"00000") == 0) sleep();    // Ambient未設定時にsleepを実行

    S = "{\"writeKey\":\""+String(Amb_Key);     // (項目)writeKey,(値)ライトキー
    S += "\",\"d1\":\"" + String(temp,2);       // (項目)d1,(値)温度
    S += "\",\"d2\":\"" + String(hum,2) + "\"}"; // (項目名)d2,(値)湿度
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
    delay(100);                                 // 送信完了の待ち時間処理
    WiFi.disconnect();                          // Wi-Fiの切断
    led_off();                                  // RGB LEDの消灯
    Serial.println("Sleep...");                 // 「Sleep」をシリアル出力表示
    esp_deep_sleep(SLEEP_P);                    // Deep Sleepモードへ移行
}
