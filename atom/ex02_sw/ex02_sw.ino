/*******************************************************************************
Example 2: ESP32 (IoTセンサ) Wi-Fi ボタン for ESP32 / ATOM / ATOM Lite
・ボタンを押下するとUDPでLAN内に文字列"Ping"を送信します。
・LINE用トークンを設定すれば、LINEアプリに「ボタンが押されました」を通知。
・別の子機となる Wi-Fi コンシェルジェ照明担当(ワイヤレスLED子機)のIPアドレスを
　設定すれば、ボタンを押下したときに子機のLEDをON、押し続けたときにOFFに制御。

                                          Copyright (c) 2021-2022 Wataru KUNINO
********************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/atom/arduino

ATOM Lite Arduino Library API 情報(本サンプルでは使用しない)：
https://docs.m5stack.com/en/api/atom/system

【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example02_sw
https://github.com/bokunimowakaru/esp/tree/master/2_example/example34_sw
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex02_sw
*******************************************************************************/

#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ
#include "esp_sleep.h"                          // ESP32用Deep Sleep ライブラリ

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

#define PIN_LED_RGB 27                          // G27 に RGB LED
#define PIN_BTN 39                              // G39 に 操作ボタン
#define PIN_BTN_GPIO_NUM GPIO_NUM_39            // G39 をスリープ解除信号へ設定
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

int wake = (int)esp_sleep_get_wakeup_cause();   // 起動理由を変数wakeに保存
int clickType = 1;                              // 操作:1=Norm,2=Double,3=Long
String btn_S[]={"No","Single","Double","Long"}; // ボタン名

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
    Serial.println("M5 SW UDP LINE LED");       // 「SW UDP」をシリアル出力表示
    Serial.printf(" Wake = %d\n", wake);        // 起動理由noをシリアル出力表示
    if(wake != ESP_SLEEP_WAKEUP_EXT0) sleep();  // ボタン以外で起動時にスリープ
    led(0,15,15);                               // LEDを水色で点灯
    clickType = get_clickType();                // ボタン操作内容を取得
    Serial.printf(" Type = %d ", clickType);    // 操作内容をシリアル出力表示
    Serial.println(btn_S[clickType]);           // ボタン名をシリアル出力表示
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイント接続
    while(WiFi.status() != WL_CONNECTED){       // 接続に成功するまで待つ
        int i = (millis()/50) % 10;             // LEDの輝度点滅用の値
        switch(clickType){
            case 1: led(i,2*(i>0),i>0);break;       // LEDを茶色で点滅
            case 2: led(i*4,0,0);  break;       // LEDを赤色で点滅
            default:led(i*4,i*2,0);break;       // LEDを橙色で点滅
        }
        if(millis() > 30000) sleep();           // 30秒超過でスリープ
        delay(50);                              // 待ち時間処理
    }
    led(0,20,0);                                // LEDを緑色で点灯
    Serial.print(WiFi.localIP());               // 本機のアドレスをシリアル出力
    Serial.print(" -> ");                       // 矢印をシリアル出力
    Serial.println(UDPTO_IP);                   // UDPの宛先IPアドレスを出力
}

void loop(){                                    // 繰り返し実行する関数
    WiFiUDP udp;                                // UDP通信用のインスタンス定義
    udp.beginPacket(UDPTO_IP, PORT);            // UDP送信先を設定
    udp.println("Ping");                        // メッセージ"Ping"を送信
    udp.endPacket();                            // UDP送信の終了(実際に送信)
    delay(200);                                 // 送信待ち時間

    HTTPClient http;                            // HTTPリクエスト用インスタンス
    http.setConnectTimeout(15000);              // タイムアウトを15秒に設定する
    String url;                                 // URLを格納する変数を生成

    if(strlen(LINE_TOKEN) > 42){                // LINE_TOKEN設定時
        url = "https://notify-api.line.me/api/notify";  // LINEのURLを代入
        Serial.println(url);                    // 送信URLを表示
        http.begin(url);                        // HTTPリクエスト先を設定する
        http.addHeader("Content-Type","application/x-www-form-urlencoded");
        http.addHeader("Authorization","Bearer " + String(LINE_TOKEN));
        http.POST("message=ボタン(" + btn_S[clickType]  + ")が押されました");
        http.end();                             // HTTP通信を終了する
    }
    if(strcmp(LED_IP,"192.168.1.0")){           // 子機IPアドレス設定時
        url = "http://" + String(LED_IP) + "/?L="; // アクセス先URL
        url += String(clickType == 1 ? 1 : 0);  // L=シングル時1、その他0
        Serial.println(url);                    // 送信URLを表示
        http.begin(url);                        // HTTPリクエスト先を設定する
        http.GET();                             // ワイヤレスLEDに送信する
        http.end();                             // HTTP通信を終了する
    }
    sleep();                                    // 下記のsleep関数を実行
}

void sleep(){                                   // スリープ実行用の関数
    delay(100);                                 // 送信完了の待ち時間処理
    WiFi.disconnect();                          // Wi-Fiの切断
    Serial.print(" Btn  = ");                   // 「Btn = 」をシリアル出力表示
    Serial.println(digitalRead(PIN_BTN));       // ボタン状態をシリアル表示
    int i = 0;                                  // ループ用の数値変数i
    while(i<100){                               // スイッチ・ボタン解除待ち
        i = digitalRead(PIN_BTN) ? i+1 : 0;     // ボタン開放時にiに1を加算
        delay(1);                               // 待ち時間処理
    }
    led_off();                                  // RGB LEDの消灯
    Serial.println("Sleep...");                 // 「Sleep」をシリアル出力表示
    esp_sleep_enable_ext0_wakeup(PIN_BTN_GPIO_NUM,0);   // 割込み設定
    esp_deep_sleep_start();                     // Deep Sleepモードへ移行
}
