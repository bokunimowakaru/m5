/*******************************************************************************
Example 7: 現在のGPS(GNSS)の位置情報を送信する
・GPSモジュール：u-blox NEO-6M NEO-6M-0-001
・ボタン長押し起動で連続動作します。

                                               Copyright (c) 2018 Wataru KUNINO
********************************************************************************
★ご注意★：屋外での視覚用にLEDの★輝度を高めに設定★してあります。
********************************************************************************
Arduino IDE 用の ESP32 開発環境のセットアップ

 1. Arduino IDE (https://www.arduino.cc/en/software/) をインストールする。
 2. Arduino IDE を起動し、[ファイル]メニュー内の[環境設定]を開き、「追加のボード
    マネージャのURL」の欄に下記のURLを追加する。
    https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
 3. [ツール]メニュー内の[ボード]からボードマネージャを開き、検索窓に「esp32」を
    入力後、esp32 by Espressif Systems をインストールする。
 4. [ツール]メニュー内の[ボード]で ESP32C3 DEev Module を選択する。
 5. ATOM(Lite)の場合は、[ツール]メニュー内の[Upload Speed]で115200を選択する。
********************************************************************************
【参考文献】
ESP32 開発環境イントール方法：
https://github.com/espressif/arduino-esp32
https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html

M5Stack ATOM用 Arduino IDE 開発環境イントール方法(本サンプルでは使用しない)：
https://docs.m5stack.com/en/quick_start/atom/arduino

ATOM Lite Arduino Library API 情報(本サンプルでは使用しない)：
https://docs.m5stack.com/en/api/atom/system

【引用コード】
https://github.com/bokunimowakaru/SORACOM-LoRaWAN/blob/master/examples/cqp_ex05_gps_bin
*******************************************************************************/

#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ
#include "esp_sleep.h"                          // ESP32用Deep Sleep ライブラリ
#include "lib_TinyGPS.h"

#define PIN_LED_RGB 27                          // G27 に RGB LED
#define PIN_BTN 39                              // G39 に 操作ボタン
#define PIN_BTN_GPIO_NUM GPIO_NUM_39            // G39 をスリープ解除信号へ設定
#define SSID "1234ABCD"                         // 無線LANアクセスポイント SSID
#define PASS "password"                         // パスワード
#define PORT 1024                               // 送信のポート番号
#define SLEEP_P 10*60*1000000ul                 // スリープ時間 10分(uint32_t)
#define DEVICE "gns_s_5,"                       // デバイス名(5字+"_"+番号+",")
#define GPS_TIMEOUT_MS 30*1000                  // GPS測定の時間制限
#define WIFI_TIMEOUT_MS GPS_TIMEOUT_MS+10*1000  // Wi-Fi接続の時間制限

/******************************************************************************
 UDP 宛先 IP アドレス設定
 ******************************************************************************
 カンマ区切りでUPD宛先IPアドレスを設定してください。
 末尾を255にすると接続ネットワーク(アクセスポイント)にブロードキャスト
 *****************************************************************************/
IPAddress UDPTO_IP = {255,255,255,255};         // UDP宛先 IPアドレス

int wake = (int)esp_sleep_get_wakeup_cause();   // 起動理由を変数wakeに保存
int clickType = 0;                              // 操作:1=Norm,2=Double,3=Long
String btn_S[]={"No","Single","Double","Long"}; // ボタン名

TinyGPS gps;                                // GPSライブラリのインスタンスを定義
float lat, lon, alt;                        // 緯度・経度・標高

struct LoRaPayload{                         // LoRaWAN送信用の変数(11バイト以下)
  uint8_t gpsHeader=0x21;                   // GPS_HEADER(uint8型 1バイト)
  int32_t lat;                              // 緯度(int32型 4バイト) 1,000,000倍
  int32_t lon;                              // 経度(int32型 4バイト) 1,000,000倍
  int16_t alt;                              // 標高(int16型 2バイト) 単位＝ｍ
};

int get_clickType(){                            // ボタン操作内容を取得する
    int btn_pre = (int)!digitalRead(PIN_BTN);   // ボタンの初期状態を取得
    while(millis()<500){                        // 500msの判定期間
        if(digitalRead(PIN_BTN)){               // ボタン状態が開放の時
            if(btn_pre == 1) btn_pre = 2;       // クリック押下→開放を保持
        }else{                                  // ボタン状態が押下の時
            if(btn_pre != 1) return 2;          // ダブルクリックと判定
        }
    }
    if(btn_pre == 1) return 3;                  // ロング・プレスと判定
    return 1;                                   // シングル・クリックと判定
}

void setup(){                                   // 起動時に一度だけ実行する関数
    pinMode(PIN_BTN,INPUT_PULLUP);              // ボタン入力の設定
    led_setup(PIN_LED_RGB);                     // RGB LEDの初期設定
    Serial.begin(115200);                       // 動作確認のためのシリアル出力
    Serial.println("M5 GNSS/GPS");              // 「GNSS」をシリアル出力表示
    Serial.printf(" Wake = %d\n", wake);        // 起動理由noをシリアル出力表示
    if(wake == ESP_SLEEP_WAKEUP_EXT0) clickType=1;  // ボタンで起動
    else if(wake != ESP_SLEEP_WAKEUP_TIMER) sleep();
    led(0,60,60);                               // LEDを水色で点灯
    clickType = get_clickType();                // ボタン操作内容を取得
    Serial.printf(" Type = %d ", clickType);    // 操作内容をシリアル出力表示
    Serial.println(btn_S[clickType]);           // ボタン名をシリアル出力表示
    setupGps();                                 // GPS初期化
    if(clickType == 2){                         // ダブルクリック時
        while(!digitalRead(PIN_BTN));           // 操作完了待ち
        delay(100);                             // チャタリング防止用
        while(digitalRead(PIN_BTN)){            // ボタン未押下状態でモニタ
            char s[128];                        // 最大文字長127文字
            boolean i = getGpsRawData(s,128);   // GPS生データを取り込む
            if(i) led(0,0,40); else led(0,0,0); // LEDの青色で状態を表示
            if(s[0] != 0) Serial.println(s);    // データがあるときは表示
        }
    }
    boolean i=1,res=0;
    while(clickType <= 1 && !res){              // 通常起動かつGPSデータ未取得時
        res = getGpsPos(gps,&lat,&lon,&alt);    // GPSデータ取得
        if(millis() > GPS_TIMEOUT_MS) sleep();  // 時間超過でスリープ
        led(0, 40 * (i^=1), 40 * i);            // LEDの水色点滅で動作表示
    }
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイント接続
    while(WiFi.status() != WL_CONNECTED){       // 接続に成功するまで待つ
        led(6*((millis()/50)%10),0,0);          // LEDを赤色で点滅
        if(millis() > WIFI_TIMEOUT_MS) sleep(); // 時間超過でスリープ
        delay(50);                              // 待ち時間処理
    }
    led(0,60,0);                                // LEDを緑色で点灯
    Serial.print(WiFi.localIP());               // 本機のアドレスをシリアル出力
    Serial.print(" -> ");                       // 矢印をシリアル出力
    Serial.println(UDPTO_IP);                   // UDPの宛先IPアドレスを出力
}

void loop(){                                    // 繰り返し実行する関数
    LoRaPayload data;                           // 送信用の構造体変数dataを定義

unsigned long chars;
unsigned short sentences;
unsigned short failed;

//    if(getGpsPos(gps,&lat,&lon,&alt)){      // GPSから位置情報を取得
    if(getGpsPos(gps,&lat,&lon,&alt,&chars,&sentences,&failed)){      // GPSから位置情報を取得
        data.lat=(int32_t)(lat*1.e6);   // 緯度を構造体変数dataへ保存
        data.lon=(int32_t)(lon*1.e6);   // 経度を構造体変数dataへ保存
        data.alt=(int16_t)(alt);        // 標高を構造体変数dataへ保存
        Serial.print("lat = ");         // 緯度をシリアル出力表示
        Serial.print(data.lat);         // ±180 000 000
        Serial.print(", lon = ");       // 経度をシリアル出力表示
        Serial.print(data.lon);         // ±180 000 000
        Serial.print(", alt = ");       // 標高をシリアル出力表示
        Serial.println(data.alt);       // 単位＝ m メートル
        delay(100);                     // シリアル出力の完了待ち
    }else{
        data.lat=0; data.lon=0; data.alt=0;
    }

    String S = String(DEVICE);             // 送信データ保持用の文字列変数
    S += String(int(data.lat)) + ", ";     // 緯度値を送信データに追記
    S += String(int(data.lon)) + ", ";     // 経度値を送信データに追記
//  S += String(int(data.alt));            // 標高値を送信データに追記
    S += String(int(data.alt)) + ", ";            // 標高値を送信データに追記
    S += String(chars) + ", ";            // 標高値を送信データに追記
    S += String(sentences) + ", ";            // 標高値を送信データに追記
    S += String(failed);            // 標高値を送信データに追記
    Serial.println(S);                          // シリアル出力表示

    WiFiUDP udp;                                // UDP通信用のインスタンス定義
    udp.beginPacket(UDPTO_IP, PORT);            // UDP送信先を設定
    udp.println(S);                             // センサ値を送信
    udp.endPacket();                            // UDP送信の終了(実際に送信)
    delay(10);                                  // 送信待ち時間
/*
    HTTPClient http;                            // HTTPリクエスト用インスタンス
    http.setConnectTimeout(15000);              // タイムアウトを15秒に設定する
    String url;                                 // URLを格納する変数を生成
*/
    if(digitalRead(PIN_BTN)==0) clickType = 0;  // ボタン押下時にclickTypeを0に
    if(clickType <= 1) sleep();                 // sleep関数を実行
}

void sleep(){                                   // スリープ実行用の関数
    WiFi.disconnect();                          // Wi-Fiの切断
    led(60,0,0);                                // LEDを赤色で点灯
    Serial.print(" Btn  = ");                   // 「Btn = 」をシリアル出力表示
    Serial.println(digitalRead(PIN_BTN));       // ボタン状態をシリアル表示
    int i = 0;                                  // ループ用の数値変数i
    while(i<100){                               // スイッチ・ボタン解除待ち
        i = digitalRead(PIN_BTN) ? i+1 : 0;     // ボタン開放時にiに1を加算
        delay(1);                               // 待ち時間処理
    }
    led_off();                                  // RGB LEDの消灯
    Serial.println("Sleep...");                 // 「Sleep」をシリアル出力表示
    delay(100);                                 // 待ち時間処理
    esp_sleep_enable_ext0_wakeup(PIN_BTN_GPIO_NUM,0);   // 割込み設定
    // esp_deep_sleep_start();                     // Deep Sleepモードへ移行
    esp_deep_sleep(SLEEP_P);                    // Deep Sleepモードへ移行
}
