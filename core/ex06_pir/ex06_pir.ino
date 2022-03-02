/*******************************************************************************
Example 6: ESP32 (IoTセンサ) Wi-Fi 人感センサ子機 for M5Stack Core
・人感センサ PIR Unit が人体などの動きを検知するとUDPブロードキャスト送信します。
・レベルメータで最終検知からの経過時間の目安が分かります。

    使用機材(例)：M5Stack Core + PIR Unit

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

#include <M5Stack.h>                            // M5Stack用ライブラリの組み込み
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ

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

#define PIN_PIR 22                              // G22にセンサ(人感/ドア)を接続
#define SSID "1234ABCD"                         // 無線LANアクセスポイントSSID
#define PASS "password"                         // パスワード
#define PORT 1024                               // 受信ポート番号
#define DEVICE "pir_s_5,"                       // 人感センサ時デバイス名
#define PIR_XOR 0                               // センサ送信値の論理反転の有無
RTC_DATA_ATTR int disp_max = 200;               // メータの最大値

/******************************************************************************
 UDP 宛先 IP アドレス設定
 ******************************************************************************
 カンマ区切りでUPD宛先IPアドレスを設定してください。
 末尾を255にすると接続ネットワーク(アクセスポイント)にブロードキャスト
 *****************************************************************************/
IPAddress UDPTO_IP = {255,255,255,255};         // UDP宛先 IPアドレス

boolean pir;                                    // 人感センサ値orドアセンサ状態
boolean trig = false;                           // 送信用トリガ
int count = 99999;                              // センサ検知時に約0.5秒毎に1増

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    pinMode(PIN_PIR,INPUT);                     // センサ接続したポートを入力に

    M5.Lcd.setBrightness(31);                   // 輝度を下げる（省エネ化）
    analogMeterInit("-dBsec.","PIR", -disp_max, 0);  // アナログ・メータの初期表示
    M5.Lcd.println("ex.06 M5Stack PIR (AS312)"); // タイトルの表示
    String S = "[   20   ]      [200]      [2000]"; // ボタン名を定義
    M5.Lcd.drawCentreString(S, 160, 208, 4);    // 文字列を表示
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
}

void loop(){                                    // 繰り返し実行する関数
    pir = digitalRead(PIN_PIR);                 // 人感センサの最新の状態を取得
    M5.update();                                // ボタン状態の取得
    int btn=M5.BtnA.wasPressed()+2*M5.BtnB.wasPressed()+4*M5.BtnC.wasPressed();
    switch(btn){
        case 1: disp_max = 20; break;           // 最大20dB秒まで表示
        case 2: disp_max = 200; break;          // 最大200dB秒まで表示
        case 4: disp_max = 2000; break;         // 最大2000dB秒まで表示
        default: btn = 0; break;
    }
    if(btn) analogMeterInit(-disp_max,0);       // ボタン操作時にグラフ初期化
    if(millis()%500 == 0 && count > 0){
        count++;                                // 500msに1回だけcountを加算
        float v = - 20. * log10(count/2);
        analogMeterNeedle(v,10);                // 経過時間に応じてメータ針を設定
    }
    boolean PIR = pir ^ PIR_XOR;                // 検知状態を1、非検知を0に
    if(PIR){
        count = 1;
        if(!trig){
            WiFi.begin(SSID,PASS);              // 無線LANアクセスポイント接続
            M5.Lcd.fillRect(0, 182, 320, 26, DARKCYAN);
            M5.Lcd.drawCentreString("Detected", 160, 184, 4);
            trig = true;
        }
    }
    if(!trig) return;                           // 送信トリガなしの時に戻る
    if(WiFi.status() != WL_CONNECTED) return;   // Wi-Fi未接続のときに戻る

    String S = String(DEVICE);                  // 送信データ保持用の文字列変数
    S += "1, " + String(int(PIR));              // 1と現在のPIR値を送信データに
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
    delay(100);                                 // 送信完了の待ち時間処理
    M5.Lcd.fillRect(0, 182, 320, 26, BLACK);    // Detectedを消す
    WiFi.disconnect();                          // Wi-Fiの切断
    trig = false;
}
