/*******************************************************************************
Example 3: ESP32 (IoTセンサ) Wi-Fi 照度計 for M5Stick C
・照度センサ から取得した照度値を送信するIoTセンサです。

    使用機材(例)：M5Stick C + HAT-DLIGHT

                                          Copyright (c) 2021-2022 Wataru KUNINO
*******************************************************************************/

#include <M5StickC.h>                           // M5StickC用ライブラリ
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ
#include "esp_sleep.h"                          // ESP32用Deep Sleep ライブラリ

#define SSID "1234ABCD"                         // 無線LANアクセスポイントSSID
#define PASS "password"                         // パスワード
#define PORT 1024                               // 送信のポート番号
#define SLEEP_P 30*1000000ul                    // スリープ時間 30秒(uint32_t)
#define DEVICE "illum_5,"                       // デバイス名(5字+"_"+番号+",")

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
    pinMode(M5_LED,OUTPUT);                     // 内蔵LED用GPIOを出力に設定
    digitalWrite(M5_LED,LOW);                   // LED ON
    M5.begin();                                 // M5StickC用Lcdライブラリの起動
    bh1750Setup(19,22);                         // 照度センサの初期化
    M5.Axp.ScreenBreath(7+2);                   // LCDの輝度を2に設定
    M5.Lcd.setRotation(1);                      // LCDを横向き表示に設定
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    digitalWrite(M5_LED,HIGH);                  // LED OFF
    analogMeterInit("lx", "Illum", 0, 1000);    // アナログ・メータの初期表示
}

void loop(){                                    // 繰り返し実行する関数
    M5.update();                                // ボタン状態の取得
    float lux = getLux();                       // 照度(lux)を取得
    if(lux < 0.) sleep();                       // 取得失敗時に末尾のsleepを実行

    M5.Axp.ScreenBreath(7 + 2 - (millis() > 3000)); // 起動後3秒以上でLCDを暗く
    analogMeterNeedle(lux,5);                   // 照度に応じてメータ針を設定
    M5.Lcd.setTextColor(BLACK,WHITE);           // 文字の色を黒、背景色を白に
    M5.Lcd.setCursor(0,0);                      // 表示位置を原点(左上)に設定
    if(WiFi.status() != WL_CONNECTED ){         // Wi-Fi未接続のとき
        M5.Lcd.printf("(%d)",WiFi.status());    // Wi-Fi状態番号を表示
        if(millis() > 30000) sleep();           // 30秒超過でスリープ
        return;                                 // loop関数を繰り返す
    }
    M5.Lcd.println(WiFi.localIP());             // 本機のアドレスをシリアル出力

    String S = String(DEVICE) + String(lux,0);  // 送信データSにデバイス名を代入
    Serial.println(S);                          // 送信データSをシリアル出力表示
    WiFiUDP udp;                                // UDP通信用のインスタンスを定義
    udp.beginPacket(UDPTO_IP, PORT);            // UDP送信先を設定
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
    while(M5.BtnA.read());                      // ボタン開放待ち
    delay(200);                                 // 送信待ち時間
    WiFi.disconnect();                          // Wi-Fiの切断
    Serial.println("Sleep...");                 // 「Sleep」をシリアル出力表示
    M5.Axp.ScreenBreath(0);                     // LCD用バックライトの消灯
    M5.Lcd.fillScreen(BLACK);                   // LCDの消去
    M5.Axp.SetLDO2(false);                      // LCDバックライト用電源OFF
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_37,0); // ボタン割込み設定(G37)
    esp_deep_sleep(SLEEP_P);                    // Deep Sleepモードへ移行
}

/******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/m5stickc/arduino

M5StickC Arduino Library API 情報 (旧モデル M5StackC 用)：
https://docs.m5stack.com/en/api/stickc/system_m5stickc

【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example06_lum
https://github.com/bokunimowakaru/esp/tree/master/2_example/example38_lum
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex03_lum
*******************************************************************************/
