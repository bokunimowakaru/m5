/*******************************************************************************
Example 3: Wi-Fi 照度計 for M5 ATOM S3
・照度センサ から取得した照度値を送信するIoTセンサです。

    使用機材(例)：M5 ATOM S3 + ATOM-HAT(ATOM-MATEに付属) + HAT-DLIGHT

                                          Copyright (c) 2021-2023 Wataru KUNINO

ボタンでのスリープ解除に未対応です。リセットボタンで解除してください。
*******************************************************************************/

#include "M5AtomS3.h"                           // ATOM S3 用ライブラリ
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
    bh1750Setup(6,5);                           // 照度センサの初期化
    M5.begin();                                 // M5StickC用Lcdライブラリの起動
//  M5.Lcd.setBrightness(200);                  // LCDの輝度設定(#if 0で未定義)
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    analogMeterInit("lx", "Illum", 0, 1000);    // アナログ・メータの初期表示
}

void loop(){                                    // 繰り返し実行する関数
    M5.update();                                // ボタン状態の取得
    float lux = getLux();                       // 照度(lux)を取得
    if(lux < 0.) sleep();                       // 取得失敗時に末尾のsleepを実行

//  if(millis()>3000) M5.Lcd.setBrightness(50); // LCDの輝度設定(#if 0で未定義)
    analogMeterNeedle(lux,5);                   // 照度に応じてメータ針を設定
    
    M5.Lcd.setTextColor(BLACK,WHITE);           // 文字の色を黒、背景色を白に
    M5.Lcd.setCursor(0,0);                      // 表示位置を原点(左上)に設定
    if(WiFi.status() != WL_CONNECTED ){         // Wi-Fi未接続のとき
        M5.Lcd.printf("(%d)",WiFi.status());    // Wi-Fi状態番号を表示
        if(millis() > 30000) sleep();           // 30秒超過でスリープ
        return;                                 // loop関数を繰り返す
    }
    M5.Lcd.println(WiFi.localIP());             // 本機のアドレスをLCDに表示
    if(M5.Btn.read() || millis()<6000) return;  // 起動後6秒以下はメータ更新

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
    delay(100);                                 // 送信完了の待ち時間処理
    WiFi.disconnect();                          // Wi-Fiの切断
//  M5.Lcd.setBrightness(0);                    // LCDの輝度設定(#if 0で未定義)
    M5.Lcd.fillScreen(BLACK);                   // LCDの消去
//  M5.Lcd.sleep();                             // LCDの電源オフ(#if 0で未定義)
    Serial.println("Sleep...");                 // 「Sleep」をシリアル出力表示
//  esp_sleep_enable_ext0_wakeup(GPIO_NUM_41,0); // ボタン割込み設定(G41)
    esp_deep_sleep(SLEEP_P);                    // Deep Sleepモードへ移行
}

/*******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/atoms3/arduino

M5StickC Arduino Library API 情報 (M5StackC 用 ※ATOM S3用ではない )：
https://docs.m5stack.com/en/api/stickc/system_m5stickc

BH1750FVI データシート 2011.11 - Rev.D (ローム)

https://github.com/m5stack/M5AtomS3

【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example06_lum
https://github.com/bokunimowakaru/esp/tree/master/2_example/example38_lum
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex03_lum
*******************************************************************************/
