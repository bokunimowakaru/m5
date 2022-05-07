/*******************************************************************************
Example 6: ESP32 (IoTセンサ) Wi-Fi 人感センサ子機 for M5Stack Core
・人感センサ PIR Unit が人体などの動きを検知するとUDPブロードキャスト送信します。
・レベルメータで最終検知からの経過時間の目安が分かります。

    使用機材(例)：M5Stack Core + PIR Unit

                                           Copyright (c) 2021-2022 Wataru KUNINO
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

/******************************************************************************
 Wi-Fi コンシェルジェ証明担当（ワイヤレスLED子機） の設定
 ******************************************************************************
 ※ex01_led または ex01_led_io が動作する、別のESP32C3搭載デバイスが必要です
    1. ex01_led/ex01_led_io搭載デバイスのシリアルターミナルでIPアドレスを確認
    2. 下記のLED_IPのダブルコート(")内に貼り付け
 *****************************************************************************/
#define LED_IP "192.168.1.0"                    // LED搭載子のIPアドレス★要設定

#define PIN_PIR 22                              // G22にセンサ(人感/ドア)を接続
#define SSID "1234ABCD"                         // 無線LANアクセスポイントSSID
#define PASS "password"                         // パスワード
#define PORT 1024                               // 受信ポート番号
#define DEVICE "pir_s_3,"                       // 人感センサ時デバイス名
#define PIR_XOR 0                               // センサ送信値の論理反転の有無
RTC_DATA_ATTR int disp_max = 80;                // メータの最大値

/******************************************************************************
 UDP 宛先 IP アドレス設定
 ******************************************************************************
 カンマ区切りでUPD宛先IPアドレスを設定してください。
 末尾を255にすると接続ネットワーク(アクセスポイント)にブロードキャスト
 *****************************************************************************/
IPAddress UDPTO_IP = {255,255,255,255};         // UDP宛先 IPアドレス

boolean pir;                                    // 人感センサ値orドアセンサ状態
boolean trig = false;                           // 送信用トリガ(Wi-Fi接続待ち)
boolean led = false;                            // ワイヤレスLED端末の状態
unsigned long base_ms = 0;                      // センサ検知時の時刻
unsigned long wifi_ms = 0;                      // Wi-Fi接続開始時刻

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    pinMode(PIN_PIR,INPUT);                     // センサ接続したポートを入力に
    M5.Lcd.setBrightness(31);                   // 輝度を下げる（省エネ化）
    analogMeterInit("-dBmsec.","PIR", -disp_max, 0);  // アナログメータ初期表示
    M5.Lcd.println("ex.06 M5Stack PIR (AS312)"); // タイトルの表示
    String S = "[60]         [80]        [100]"; // ボタン名を定義
    M5.Lcd.drawCentreString(S, 165, 208, 4);    // ボタン名を表示
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
}

void loop(){                                    // 繰り返し実行する関数
    pir = digitalRead(PIN_PIR);                 // 人感センサの最新の状態を取得
    M5.update();                                // ボタン状態の取得
    delay(1);                                   // ボタンの誤作動防止
    int btn=M5.BtnA.wasPressed()+2*M5.BtnB.wasPressed()+4*M5.BtnC.wasPressed();
    switch(btn){
        case 1: disp_max = 60; break;           // 最大60dBミリ秒まで表示
        case 2: disp_max = 80; break;           // 最大80dBミリ秒まで表示
        case 4: disp_max = 100; break;          // 最大100dBミリ秒まで表示
        default: btn = 0; break;
    }
    if(btn) analogMeterInit(-disp_max,0);       // ボタン操作時にグラフ初期化
    float v = -20.*log10(millis()-base_ms);     // 経過時間を-dBミリ秒に変換
    analogMeterNeedle(v,10);                    // 経過時間に応じてメータ値設定
    delay(33);                                  // 表示の点滅低減
    boolean PIR = pir ^ PIR_XOR;                // 検知状態を1、非検知を0に
    if(PIR){                                    // 検知状態の時
        analogMeterNeedle(0,1);                 // 針位置を最大値(0・右)に移動
        if(!trig){                              // Wi-Fi接続待ちではないとき
            WiFi.begin(SSID,PASS);              // 無線LANアクセスポイント接続
            wifi_ms = millis();                 // 接続処理の開始時刻を保持
            M5.Lcd.fillRect(0, 182, 320, 26, DARKCYAN);       // 背景色の設定
            M5.Lcd.drawCentreString("Detected", 160, 184, 4); // 検知をLCD表示
            trig = true;                        // Wi-Fi接続待ち状態を保持
            led = true;                         // ワイヤレスLチカ端末をON
        }
        base_ms = millis()-1;                   // 人感センサの検知時刻を保持
    }
    if(!trig && led && (v < -disp_max)){        // LEDのOFF制御判定部
        WiFi.begin(SSID,PASS);                  // 無線LANアクセスポイント接続
        wifi_ms = millis();                     // 接続処理の開始時刻を保持
        trig = true;                            // Wi-Fi接続待ち状態を保持
        led = false;                            // ワイヤレスLチカ端末をOFF
    }
    if(trig && millis() - wifi_ms > 5000){      // Wi-Fi未接続で5秒以上経過
        trig = false;                           // Wi-Fi接続待ち状態を解除
        WiFi.disconnect();                      // Wi-Fiの切断
        M5.Lcd.fillRect(0, 182, 320, 26, RED);  // Detectedを消す
        M5.Lcd.drawCentreString("Wi-Fi ERROR", 160, 184, 4); // エラー表示
    }
    if(trig && WiFi.status() == WL_CONNECTED){  // 送信トリガありWi-Fi接続状態
        String S = String(DEVICE);              // 送信データ保持用の文字列変数
        S += "1, " + String(int(PIR));          // 1と現在のPIR値を送信データに
        WiFiUDP udp;                            // UDP通信用のインスタンスを定義
        udp.beginPacket(UDPTO_IP, PORT);        // UDP送信先を設定
        udp.println(S);                         // センサ値を送信
        udp.endPacket();                        // UDP送信の終了(実際に送信する)
        delay(10);                              // 送信待ち時間

        HTTPClient http;                        // HTTPリクエスト用インスタンス
        http.setConnectTimeout(15000);          // タイムアウトを15秒に設定する
        String url;                             // URLを格納する文字列変数を生成
        if(led && strlen(LINE_TOKEN) > 42){     // LINE_TOKEN設定時
            url = "https://notify-api.line.me/api/notify";  // LINEのURLを代入
            http.begin(url);                    // HTTPリクエスト先を設定する
            http.addHeader("Content-Type","application/x-www-form-urlencoded");
            http.addHeader("Authorization","Bearer " + String(LINE_TOKEN));
            http.POST("message=センサが反応しました。(" + S.substring(8) + ")");
            http.end();                         // HTTP通信を終了する
        }
        if(strcmp(LED_IP,"192.168.1.0")){       // 子機IPアドレス設定時
            url = "http://" + String(LED_IP) + "/?L="; // アクセス先URL
            url += String(led ? 1 : 0);         // true時1、false時0
            http.begin(url);                    // HTTPリクエスト先を設定する
            http.GET();                         // ワイヤレスLEDに送信する
            http.end();                         // HTTP通信を終了する
        }
        delay(100);                             // 送信完了待ち＋連続送信防止
        WiFi.disconnect();                      // Wi-Fiの切断
        while(digitalRead(PIN_PIR) ^ PIR_XOR) delay(100); // センサの解除待ち
        M5.Lcd.fillRect(0, 182, 320, 26, BLACK); // Detectedを消す
        trig = false;                           // Wi-Fi接続待ち状態を解除
    }
}

/*******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/m5core/arduino

M5Stack Arduino Library API 情報：
https://docs.m5stack.com/en/api/core/system

SB412A データシート (NANYANG SENBA OPTICAL AND ELECTRONIC CO. LTD.)

BS612 AS612 データシート (NANYANG SENBA OPTICAL AND ELECTRONIC CO. LTD.)

【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example11_pir
https://github.com/bokunimowakaru/esp/tree/master/2_example/example43_pir
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex06_pir
*******************************************************************************/
