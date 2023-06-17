/*******************************************************************************
Example 5: ESP32 (IoTセンサ) Wi-Fi 温湿度計 SENSIRION製 SHT30/SHT31/SHT35 版
                                                    for ESP32 / ATOM / ATOM Lite

・デジタルI2Cインタフェース搭載センサから取得した温湿度を送信するIoTセンサです。

    使用機材(例)：M5Stack CORE INK + ENV II/III HAT

    ESP32 のI2Cポート:
        SHT30/SHT31/SHT35 SDAポート G25
        SHT30/SHT31/SHT35 SCLポート G26    設定方法＝shtSetup(SDA,SCL)

注意: ENV HATのバージョンによって搭載されているセンサが異なります。
      このプログラムは SHT30 用です。ENV HAT には対応していません。

ENV HAT     DHT12 + BMP280 + BMM150
ENV II HAT  SHT30 + BMP280 + BMM150
ENV III HAT SHT30 + QMP6988
*******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/coreink/arduino

M5Stack Arduino Library API 情報：
https://docs.m5stack.com/en/api/coreink/system_api

【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example09_hum_sht31
https://github.com/bokunimowakaru/esp/tree/master/2_example/example41_hum_sht31
https://github.com/bokunimowakaru/m5s/tree/master/example04d_temp_hum_sht
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex05_hum

                                          Copyright (c) 2016-2023 Wataru KUNINO
*******************************************************************************/

#include <M5CoreInk.h>                          // M5Stack用ライブラリ組み込み
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ
// #include "esp_sleep.h"                       // ESP32用Deep Sleep ライブラリ

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

Ink_Sprite InkPageSprite(&M5.M5Ink);            // M5Inkインスタンス作成
int eInk_x = 0;                                 // E-Ink表示用のX座標
int eInk_y = 0;                                 // E-Ink表示用のY座標

void eInk_println(){                            // E-Inkの改行処理
    eInk_x = 0;                                 // (改行処理)X座標を左端へ
    eInk_y += 16;                               // (改行処理)Y座標を下の行へ
    if(eInk_y >= 200) eInk_y = 200 - 16;        // 最下段を超えた時に最下段へ
}

void eInk_print(String text){                   // E-Inkに文字列を表示する
    char c[2];
    InkPageSprite.creatSprite(0,0,200,200);     // 画像用バッファの作成
    for(int i=0; i < text.length(); i++){       // 文字数分の繰り返し処理
        text.substring(i).toCharArray(c, 2);    // 1文字+終端の取り出し
        if(c[0] < 0x20 || c[0] >= 0x7f) continue;   // 表示不可文字の処理を排除
        InkPageSprite.drawChar(eInk_x,eInk_y,c[0]); // バッファに文字を描画
        eInk_x += 8;                            // 座標1文字分
        if(eInk_x >= 200){                      // X座標が右端を超えた時
            eInk_println();                     // 改行処理
        }
    }
    InkPageSprite.pushSprite();                 // push the sprite.
}

void eInk_println(uint32_t ip){                 // IPアドレスを表示する
    char s[16];
    sprintf(s,"%d.%d.%d.%d",ip&255,(ip>>8)&255,(ip>>16)&255,(ip>>24)&255);
    eInk_print(String(s));                      // eInk_printで文字列を表示
    eInk_println();                             // 改行
}


void eInk_println(String text){                 // E-Inkに文字列を表示する
    eInk_print(text);                           // eInk_printで文字列を表示
    eInk_x = 0;                                 // (改行処理)X座標を左端へ
    eInk_y += 16;                               // (改行処理)Y座標を下の行へ
    if(eInk_y >= 200) eInk_y = 200 - 16;        // 最下段を超えた時に最下段へ
}

int batt_mv(){                                  // 電池電圧確認
    // 　　参考文献 M5 https://docs.m5stack.com/en/core/coreink
    // 　　R41 = 20K, R42 = 5.1K, ADC=GPIO35,   [BAT]--[R41]--*--[R42]--GND
    int PIN_AIN = 35;                           // 電池電圧取得用のADCポート
    float adc;                                  // ADC値の代入用
    pinMode(PIN_AIN, ANALOG);                   // GPIO35をアナログ入力に
    adc = analogRead(PIN_AIN) * 3300./4095.;    // AD変換器から値を取得
    adc /= 5.1 / (20 + 5.1);                    // 抵抗分圧の逆数
    return (int)(adc + 0.5);                    // 電圧値(mV)を整数で応答
}

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    M5.M5Ink.isInit();                          // E-Inkの初期化
    M5.M5Ink.clear();                           // E-Inkを消去
    eInk_println("Example 5 HUM");              // 「Example 5 HUM」を表示
    eInk_println("BAT= " + String(batt_mv()) +" mV"); // 電池電圧をE-Inkに表示
    shtSetup(25,26);                            // 湿度センサの初期化

    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    while(WiFi.status() != WL_CONNECTED){       // 接続に成功するまで待つ
        eInk_print(".");                        // 接続試行中表示
        if(millis() > 30000) sleep();           // 30秒超過でスリープ
        delay(500);                             // 待ち時間処理
    }
    eInk_println(WiFi.localIP());               // 本機のアドレスをE-Inkに表示
}

void loop(){                                    // 繰り返し実行する関数
    float temp = getTemp();                     // 温度を取得して変数tempに代入
    float hum = getHum();                       // 湿度を取得して変数humに代入
    int batt = batt_mv();                       // 電池電圧を取得してbattに代入
    if(temp < -100. || hum < 0.) sleep();       // 取得失敗時に末尾のsleepを実行

    String S = String(DEVICE);                  // 送信データSにデバイス名を代入
    S += String(temp,1) + ", ";                 // 変数tempの値を追記
    S += String(hum,1) + ", ";                 // 変数humの値を追記
    S += String(batt);                          // 変数battの値を追記
    eInk_println(S);                            // 送信データSをE-Ink表示
    WiFiUDP udp;                                // UDP通信用のインスタンスを定義
    udp.beginPacket(UDPTO_IP, PORT);            // UDP送信先を設定
    udp.println(S);                             // 送信データSをUDP送信
    udp.endPacket();                            // UDP送信の終了(実際に送信する)
    if(strcmp(Amb_Id,"00000") == 0) sleep();    // Ambient未設定時にsleepを実行

    S = "{\"writeKey\":\""+String(Amb_Key);     // (項目)writeKey,(値)ライトキー
    S += "\",\"d1\":\"" + String(temp,2);       // (項目)d1,(値)温度
    S += "\",\"d2\":\"" + String(hum,2);        // (項目)d2,(値)湿度
    S += "\",\"d3\":\"" + String(batt) + "\"}"; // (項目)d3,(値)電圧
    HTTPClient http;                            // HTTPリクエスト用インスタンス
    http.setConnectTimeout(15000);              // タイムアウトを15秒に設定する
    String url = "http://ambidata.io/api/v2/channels/"+String(Amb_Id)+"/data";
    http.begin(url);                            // HTTPリクエスト先を設定する
    http.addHeader("Content-Type","application/json"); // JSON形式を設定する
    eInk_println(url);                          // 送信URLを表示
    http.POST(S);                               // センサ値をAmbientへ送信する
    http.end();                                 // HTTP通信を終了する
    sleep();                                    // 下記のsleep関数を実行
}

void sleep(){                                   // スリープ実行用の関数
    delay(100);                                 // 送信完了の待ち時間処理
    WiFi.disconnect();                          // Wi-Fiの切断
    digitalWrite(LED_EXT_PIN, HIGH);            // LED消灯
    if(batt_mv() > 3300){                       // 電圧が3300mV以上のとき
        int sec = (int)(SLEEP_P/1000000ul);     // 秒に変換
        eInk_println("Sleeping for " + String(sec) + " Seconds");
        // esp_deep_sleep(SLEEP_P);             // Deep Sleepモードへ移行
        M5.shutdown(sec);                       // タイマー・スリープ
    }   // else:
    eInk_println("Power OFF");                  // E-Inkへメッセージを表示
    M5.shutdown();                              // 電源OFF
}
