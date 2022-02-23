/*******************************************************************************
Example 7: GPS(GNSS)の位置情報を取得し、Wi-Fiで送信する
・動作確認済みGPSモジュール：u-blox NEO-6M NEO-6M-0-001, 杭州中科微电子 AT6558
・ボタン長押し起動で連続動作します。
・ダブル・クリックでGPSから受信したNMEA情報をUDP送信します

    使用機材(例)：ATOM/ATOM Lite + Mini GPS/BDS Unit (AT6558)

                                               Copyright (c) 2022 Wataru KUNINO
********************************************************************************
★ご注意★・屋外での視覚用にLEDの★輝度を高めに設定★してあります。
          ・GPS Unit を ATOM の Grove互換端子に接続してください。
          ・GPS Unitの電源を入れてから位置情報が得られるまで数分以上を要します。
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
#define SLEEP_P 30*1000000ul                    // スリープ時間 30秒(uint32_t)
#define DEVICE "gns_s_5,"                       // デバイス名(5字+"_"+番号+",")
#define GPS_TIMEOUT_MS 6*1000                   // GPS測定の時間制限
#define WIFI_TIMEOUT_MS GPS_TIMEOUT_MS+10*1000  // Wi-Fi接続の時間制限

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

int wake = (int)esp_sleep_get_wakeup_cause();   // 起動理由を変数wakeに保存
int clickType = 0;                              // 操作:1=Norm,2=Double,3=Long
String btn_S[]={"No","Single","Double","Long"}; // ボタン名

TinyGPS gps;                                    // GPSライブラリのインスタンス
float lat=0., lon=0., alt=0.;                   // 緯度・経度・標高
unsigned short sat=0;
unsigned long hdop=0, age=0;

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
    led(0,60,60);                               // LEDを水色で点灯
    Serial.begin(115200);                       // 動作確認のためのシリアル出力
    if(wake == ESP_SLEEP_WAKEUP_EXT0) clickType=1;  // ボタンで起動
    else if(wake != ESP_SLEEP_WAKEUP_TIMER) sleep();
    clickType = get_clickType();                // ボタン操作内容を取得
    while(!digitalRead(PIN_BTN));               // 操作完了待ち
    delay(100);                                 // チャタリング防止用

    WiFi.mode(WIFI_OFF);                        // 無線LANをOFFモードに設定
    Serial.println("M5 GNSS/GPS");              // 「GNSS」をシリアル出力表示
    Serial.printf(" Wake = %d\n", wake);        // 起動理由noをシリアル出力表示
    Serial.printf(" Type = %d ", clickType);    // 操作内容をシリアル出力表示
    Serial.println(btn_S[clickType]);           // ボタン名をシリアル出力表示
    setupGps();                                 // GPS初期化

    boolean i=1,res=0;
    while(clickType <= 1 && !res){              // 通常起動かつGPSデータ未取得時
        res = getGpsPos(gps,&lat,&lon,&alt);    // GPSデータ取得
        if(millis() > GPS_TIMEOUT_MS) sleep();  // 時間超過でスリープ
        led(0, 40 * (i^=1), 40 * i);            // LEDの水色点滅で動作表示
    }

    if(clickType <= 1){                         // シングル・クリック時
        while(!getGpsPos(gps,&lat,&lon,&alt,&sat,&hdop,&age)){  // GPS情報取得
            led(6*((millis()/50)%10),0,0);      // LEDを赤色で点滅
            if(millis() > GPS_TIMEOUT_MS || !digitalRead(PIN_BTN)) sleep();
            delay(50);                          // 待ち時間処理
        }
        Serial.println("GPS: " + String(sat) + " satellites");  // ログをLCD表示
    }
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイント接続

    if(clickType == 2){                         // ダブルクリック時
        while(digitalRead(PIN_BTN)){            // ボタン未押下状態でモニタ
            char s[128];                        // 最大文字長127文字
            boolean i = getGpsRawData(s,128);   // GPS生データを取り込む
            if(i) led(0,0,40); else led(0,0,0); // LEDの青色で状態を表示
            if(s[0] != 0) Serial.println(s);    // データがあるときは表示
        }
        sleep();
    }

    while(WiFi.status() != WL_CONNECTED){       // 接続に成功するまで待つ
        led(6*((millis()/50)%10),0,0);          // LEDを赤色で点滅
        if(millis() > WIFI_TIMEOUT_MS) sleep(); // 時間超過でスリープ
        delay(50);                              // 待ち時間処理
    }
    Serial.print(WiFi.localIP());               // 本機のアドレスをシリアル出力
    Serial.print(" -> ");                       // 矢印をシリアル出力
    Serial.println(UDPTO_IP);                   // UDPの宛先IPアドレスを出力
    led(0,60,0);                                // LEDを緑色で点灯
}

void loop(){                                    // 繰り返し実行する関数
    getGpsPos(gps,&lat,&lon,&alt,&sat,&hdop,&age);  // GPSから位置情報を取得
    if(alt>1000) alt = 0.;                      // 測定不可時の対応
    String S = String(DEVICE);                  // 送信データ保持用の文字列変数
    S += String(lat,6) + ", ";                  // 緯度値を送信データに追記
    S += String(lon,6) + ", ";                  // 経度値を送信データに追記
    S += String(alt,0) + ", ";                  // 標高値を送信データに追記
    S += String(sat) + ", ";                    // 衛生数を送信データに追記
    S += String(hdop) + ", ";                   // DHOP値を送信データに追記
    S += String(age);                           // 取得経過時間を追記
    Serial.println("GPS: " + S);                // LCDに表示

    WiFiUDP udp;                                // UDP通信用のインスタンス定義
    udp.beginPacket(UDPTO_IP, PORT);            // UDP送信先を設定
    udp.println(S);                             // センサ値を送信
    udp.endPacket();                            // UDP送信の終了(実際に送信)
    delay(10);                                  // 送信待ち時間

    if(strcmp(Amb_Id,"00000") != 0){            // Ambient設定時
        S = "{\"writeKey\":\""+String(Amb_Key); // (項目)writeKey,(値)ライトキー
        S+= "\",\"d1\":\"" + String(alt,0);     // (項目)d1,(値)標高値
        S+= "\",\"lat\":\"" + String(lat,6);    // (項目)lat,(値)緯度値
        S+= "\",\"lng\":\"" + String(lon,6);    // (項目)lng,(値)経度値
        S+= "\"}";
        HTTPClient http;                        // HTTPリクエスト用インスタンス
        http.setConnectTimeout(15000);          // タイムアウトを15秒に設定する
        String url="http://ambidata.io/api/v2/channels/"+String(Amb_Id)+"/data";
        http.begin(url);                        // HTTPリクエスト先を設定する
        http.addHeader("Content-Type","application/json"); // JSON形式を設定する
        Serial.println(url);                    // 送信URLを表示
        http.POST(S);                           // GPS情報をAmbientへ送信する
        http.end();                             // HTTP通信を終了する
    }
    if(digitalRead(PIN_BTN)==0) clickType = 0;  // ボタン押下時にclickTypeを0に
    if(clickType <= 1) sleep();                 // sleep関数を実行
}

void sleep(){                                   // スリープ実行用の関数
    WiFi.disconnect();                          // Wi-Fiの切断
    led(60,0,0);                                // LEDを赤色で点灯
    Serial.print(" Btn  = ");                   // 「Btn = 」をシリアル出力表示
    Serial.println(!digitalRead(PIN_BTN));      // ボタン状態をシリアル表示
    int i = 0;                                  // ループ用の数値変数i
    while(i<100){                               // スイッチ・ボタン解除待ち
        i = digitalRead(PIN_BTN) ? i+1 : 0;     // ボタン開放時にiに1を加算
        delay(1);                               // 待ち時間処理
    }
    led_off();                                  // RGB LEDの消灯
    Serial.println("Sleep...");                 // 「Sleep」をシリアル出力表示
    delay(100);                                 // 待ち時間処理
    esp_sleep_enable_ext0_wakeup(PIN_BTN_GPIO_NUM,0);   // 割込み設定
    esp_deep_sleep(SLEEP_P);                    // Deep Sleepモードへ移行
}
