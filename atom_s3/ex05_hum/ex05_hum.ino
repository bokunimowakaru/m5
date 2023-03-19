/*******************************************************************************
Example 5: Wi-Fi 温湿度計 SENSIRION製 SHT30/SHT31/SHT35 版
・デジタルI2Cインタフェース搭載センサから取得した温湿度を送信するIoTセンサです。

    使用機材(例)：M5 ATOM S3 + ATOM-HAT(ATOM-MATEに付属) + ENV II/III HAT

注意: ENV HATのバージョンによって搭載されているセンサが異なります。
      このプログラムは SHT30 用です。ENV HAT には対応していません。

ENV HAT     DHT12 + BMP280 + BMM150 ※非対応
ENV II HAT  SHT30 + BMP280 + BMM150
ENV III HAT SHT30 + QMP6988

                                          Copyright (c) 2016-2023 Wataru KUNINO
*******************************************************************************/

#include "M5AtomS3.h"                           // ATOM S3 用ライブラリ
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ
#include "esp_sleep.h"                          // ESP32用Deep Sleep ライブラリ

#define SSID "1234ABCD"                         // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
#define PORT 1024                               // 送信のポート番号
#define SLEEP_P 30*1000000ul                    // スリープ時間 30秒(uint32_t)
#define DEVICE "humid_5,"                       // デバイス名(5字+"_"+番号+",")
RTC_DATA_ATTR int disp = 0;                     // メータ表示番号 0～
unsigned long lcd_ms = millis();                // 表示してからの経過時間

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

void disp_init(){ switch(disp){                 // アナログ・メータの初期表示用
    case 0: analogMeterInit("Celsius", "Temp.", 0, 40); break;
    case 1: analogMeterInit("RH%", "Humi.", 0, 100); break;
    default: analogMeterInit("mA", "Batt.I", 0, 160);
}}

void setup(){                                   // 起動時に一度だけ実行する関数
    shtSetup(6,5);                              // 湿度センサの初期化
    M5.begin();                                 // M5StickC用Lcdライブラリの起動
//  M5.Lcd.setBrightness(200);                  // LCDの輝度設定(#if 0で未定義)
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    disp_init();                                // アナログメータ表示の初期化
}

void loop(){                                    // 繰り返し実行する関数
    M5.update();                                // ボタン状態の取得
    if(M5.Btn.wasPressed()){                    // (過去に)ボタンが押された時
        disp++;                                 // 画面番号を更新
        if(disp>1) disp=0;                      // 画面数1超過時に0に戻す
        disp_init();                            // アナログメータ表示の初期化
        lcd_ms = millis();                      // スリープ時間の猶予管理用
    }
    if(millis()%500) return;                    // 以下は500msに1回だけ実行する

    float temp = getTemp();                     // 温度を取得して変数tempに代入
    float hum = getHum();                       // 湿度を取得して変数humに代入
    if(temp < -100. || hum < 0.) sleep();       // 取得失敗時に末尾のsleepを実行

//  if(millis()>3000) M5.Lcd.setBrightness(50); // LCDの輝度設定(#if 0で未定義)
    switch(disp){                               // 画面番号に応じて針を動かす
        case 0: analogMeterNeedle(temp,5); break;   // 温度メータ
        case 1: analogMeterNeedle(hum,5); break;    // 湿度メータ
    }
    M5.Lcd.setTextColor(WHITE,BLACK);           // 文字の色を黒、背景色を白に
    M5.Lcd.setCursor(0,72);                     // 文字の表示位置を原点に設定
    if(WiFi.status() != WL_CONNECTED ){         // Wi-Fi未接続のとき
        M5.Lcd.printf("(%d)",WiFi.status());    // Wi-Fi状態番号を表示
        if(millis() > 30000) sleep();           // 30秒超過でスリープ
        return;                                 // loop関数を繰り返す
    }
    M5.Lcd.println(WiFi.localIP());             // 本機のアドレスをシリアル出力
    if(millis() - lcd_ms < 6000) return;

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
    M5.Lcd.println("Sleep...");                 // 「Sleep」をシリアル出力表示
    delay(100);                                 // 送信完了の待ち時間処理
    WiFi.disconnect();                          // Wi-Fiの切断
    while(M5.Btn.read());                       // ボタン開放待ち
//  M5.Lcd.setBrightness(0);                    // LCDの輝度設定(#if 0で未定義)
//  M5.Lcd.fillScreen(BLACK);                   // LCDの消去
//  M5.Lcd.sleep();                             // LCDの電源オフ(#if 0で未定義)
//  esp_sleep_enable_ext0_wakeup(GPIO_NUM_41,0); // ボタン割込み設定(G41)
    esp_deep_sleep(SLEEP_P);                    // Deep Sleepモードへ移行
}

/******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/atoms3/arduino

M5StickC Arduino Library API 情報 (M5StackC 用 ※ATOM S3用ではない )：
https://docs.m5stack.com/en/api/stickc/system_m5stickc

https://github.com/m5stack/M5AtomS3

【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example09_hum_sht31
https://github.com/bokunimowakaru/esp/tree/master/2_example/example41_hum_sht31
https://github.com/bokunimowakaru/m5s/tree/master/example04d_temp_hum_sht
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex05_hum
*******************************************************************************/
