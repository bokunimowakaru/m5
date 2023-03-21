/*******************************************************************************
Example 2: Wi-Fi ボタン for for M5 ATOM S3
・ボタンを押下するとUDPでLAN内に文字列"Ping"または"Pong"を送信します。
・ON状態の時にボタンを押すと"Pong"を送信します。
・OFF状態の時にボタンを押すと"Pong"を送信します。
・LINE用トークンを設定すれば、LINEアプリに「ボタンが押されました」等を通知。
・別の子機となる Wi-Fi コンシェルジェ証明担当(ワイヤレスLED子機)のIPアドレスを
　設定すれば、ボタンを押下したときに子機のLEDをON／OFF制御。

    使用機材(例)：M5 ATOM S3

                                          Copyright (c) 2021-2023 Wataru KUNINO
*******************************************************************************/

#include "M5AtomS3.h"                           // ATOM S3 用ライブラリ
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ
#include "on_sw_bmp.h"                          // ON状態のスイッチのJPEGデータ
#include "off_sw_bmp.h"                         // OFF状態のスイッチのJPEGデータ

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
 Wi-Fi コンシェルジェ照明担当（ワイヤレスLED子機） の設定
 ******************************************************************************
 ※ex01_led または ex01_led_io が動作する、別のESP32C3搭載デバイスが必要です
    1. ex01_led/ex01_led_io搭載デバイスのシリアルターミナルでIPアドレスを確認
    2. 下記のLED_IPのダブルコート(")内に貼り付け
 *****************************************************************************/
#define LED_IP "192.168.1.0"                    // LED搭載子のIPアドレス★要設定

/******************************************************************************
 Wi-Fi 無線アクセスポイントの設定
 *****************************************************************************/
#define SSID "1234ABCD"                         // 無線LANアクセスポイント SSID
#define PASS "password"                         // パスワード
#define PORT 1024                               // 送信のポート番号

/******************************************************************************
 UDP 宛先 IP アドレス設定
 ******************************************************************************
 カンマ区切りでUPD宛先IPアドレスを設定してください。
 末尾を255にすると接続ネットワーク(アクセスポイント)にブロードキャスト
 *****************************************************************************/
IPAddress UDPTO_IP = {255,255,255,255};         // UDP宛先 IPアドレス

/******************************************************************************
 プログラム本体
 *****************************************************************************/

String btn_S[]={"No","OFF","ON"};               // 送信要否状態0～3の名称
byte btn_stat = 0;                              // スイッチ状態

int btnUpdate(){                                // ボタン状態に応じて画面切換
    M5.update();                                // M5Stack用IO状態の更新
    delay(1);                                   // ボタンの誤作動防止用
    int tx_en = 0;                              // 送信要否tx_en(0:送信無効)
    if( M5.Btn.wasPressed() ){                  // ボタンが押されたとき
        btn_stat ^= 1;                          // ボタン状態の反転
        if(!btn_stat){
            M5.Lcd.drawBitmap(0,0,128,128,off_sw_bmp); // LCDに画像off_swを表示
        }else{
            M5.Lcd.drawBitmap(0,0,128,128,on_sw_bmp);  // LCDに画像on_swを表示
        }
        tx_en = 1 + btn_stat;                   // 送信要否tx_en(1:OFFを送信)
    }
    if(tx_en) M5.Lcd.setCursor(0, 0);           // LCD文字表示位置を原点に
    return tx_en;                               // 送信要否を応答する
}

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5StickC用Lcdライブラリの起動
//  M5.Lcd.setBrightness(200);                  // LCDの輝度設定(#if 0で未定義)
    M5.Lcd.setRotation(2);                      // LCDを縦向き表示に設定
    M5.Lcd.drawBitmap(0,0,128,128,off_sw_bmp);  // LCDに画像off_swを表示
    M5.Lcd.setTextColor(BLACK);                 // 文字色を黒に
    M5.Lcd.println("ex02 SW UDP");              // 「SW UDP」をシリアル出力表示
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイント接続
    while(WiFi.status() != WL_CONNECTED){       // 接続に成功するまで待つ
        delay(50);                              // 待ち時間処理
        btnUpdate();                            // ボタン状態を確認
    }
    M5.Lcd.println(WiFi.localIP());             // 本機のアドレスをシリアル出力
}

void loop(){                                    // 繰り返し実行する関数
    int tx_en = btnUpdate();                    // ボタン状態と送信要否の確認
    if(tx_en==0) return;                        // 送信要求が無い(0)の時に戻る

    WiFiUDP udp;                                // UDP通信用のインスタンス定義
    udp.beginPacket(UDPTO_IP, PORT);            // UDP送信先を設定
    M5.Lcd.print("udp: ");                      // 「udp:」をLCDに表示
    if(tx_en == 1){                             // OFFを送信の時
        udp.println("Pong");                    // メッセージ"Pong"を送信
        M5.Lcd.println("Pong");                 // "Pong"をLCD表示
    }else{                                      // その他の送信の時
        udp.println("Ping");                    // メッセージ"Ping"を送信
        M5.Lcd.println("Ping");                 // "Ping"をLCD表示
    }
    udp.endPacket();                            // UDP送信の終了(実際に送信)
    delay(200);                                 // 送信待ち時間

    HTTPClient http;                            // HTTPリクエスト用インスタンス
    http.setConnectTimeout(15000);              // タイムアウトを15秒に設定する
    String url;                                 // URLを格納する変数を生成

    if(strlen(LINE_TOKEN) > 42){                // LINE_TOKEN設定時
        url = "https://notify-api.line.me/api/notify";  // LINEのURLを代入
        M5.Lcd.println("notify-api.line.me");   // LINEへの送信表示
        http.begin(url);                        // HTTPリクエスト先を設定する
        http.addHeader("Content-Type","application/x-www-form-urlencoded");
        http.addHeader("Authorization","Bearer " + String(LINE_TOKEN));
        http.POST("message=ボタン(" + btn_S[tx_en]  + ")が押されました");
        http.end();                             // HTTP通信を終了する
    }
    if(strcmp(LED_IP,"192.168.1.0")){           // 子機IPアドレス設定時
        url = "http://" + String(LED_IP) + "/?L="; // アクセス先URL
        url += String(tx_en == 1 ? 0 : 1);      // L=OFF時0、その他1
        M5.Lcd.println(LED_IP);                 // 送信IPアドレスをLCD表示
        http.begin(url);                        // HTTPリクエスト先を設定する
        http.GET();                             // ワイヤレスLEDに送信する
        http.end();                             // HTTP通信を終了する
    }
}

/*******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/atoms3/arduino

M5StickC Arduino Library API 情報 (M5StackC 用 ※ATOM S3用ではない )：
https://docs.m5stack.com/en/api/stickc/system_m5stickc

M5Atom S3用ライブラリ(GitHub)：
https://github.com/m5stack/M5AtomS3

【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example02_sw
https://github.com/bokunimowakaru/esp/tree/master/2_example/example34_sw
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex02_sw
*******************************************************************************/
