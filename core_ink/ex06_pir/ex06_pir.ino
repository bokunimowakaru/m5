/*******************************************************************************
Example 6: ESP32 (IoTセンサ) 【Wi-Fi 人感センサ子機】ディープスリープ版
                                                           for M5Stack CORE INK

・人感センサ PIR HAT が人体などの動きを検知すると、ディープスリープから復帰し、
　UDPブロードキャストでセンサ値を送信します。

    M5Stack CORE INK + PIR HAT(AS312) に対応
    
                                           Copyright (c) 2016-2023 Wataru KUNINO
*******************************************************************************/

#include <M5CoreInk.h>                          // M5Stack用ライブラリ組み込み
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ
#include "esp_sleep.h"                          // ESP32用Deep Sleep ライブラリ
#include "driver/rtc_io.h"

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

#define PIN_PIR 36                              // G36にセンサ(人感/ドア)を接続
#define PIN_PIR_GPIO_NUM GPIO_NUM_36            // G36 をスリープ解除信号へ設定
#define SSID "1234ABCD"                         // 無線LANアクセスポイントSSID
#define PASS "password"                         // パスワード
#define PORT 1024                               // 受信ポート番号
#define DEVICE "pir_s_5,"                       // 人感センサ時デバイス名
#define PIR_XOR 0                               // センサ送信値の論理反転の有無

/******************************************************************************
 UDP 宛先 IP アドレス設定
 ******************************************************************************
 カンマ区切りでUPD宛先IPアドレスを設定してください。
 末尾を255にすると接続ネットワーク(アクセスポイント)にブロードキャスト
 *****************************************************************************/
IPAddress UDPTO_IP = {255,255,255,255};         // UDP宛先 IPアドレス

RTC_DATA_ATTR boolean PIR;                      // pir値のバックアップ保存用
boolean pir;                                    // 人感センサ値orドアセンサ状態
int wake = (int)esp_sleep_get_wakeup_cause();   // 起動理由を変数wakeに保存

int batt_mv(){                                  // 電池電圧確認
    int PIN_AIN = 35;                           // 電池電圧取得用のADCポート
    float adc;                                  // ADC値の代入用
    analogSetAttenuation(ADC_2_5db);            // ADC 0.1V～1.25V入力用
    pinMode(PIN_AIN, ANALOG);                   // GPIO35をアナログ入力に
    adc = analogReadMilliVolts(PIN_AIN);        // AD変換器から値を取得
    adc /= 5.1 / (20 + 5.1);                    // 抵抗分圧の逆数
    return (int)(adc + 0.5);                    // 電圧値(mV)を整数で応答
}

void setup(){                                   // 起動時に一度だけ実行する関数
    pinMode(PIN_PIR,INPUT);                     // センサ接続したポートを入力に
    pir = digitalRead(PIN_PIR);                 // 人感センサの状態を取得

    M5.begin();                                 // M5Stack用ライブラリの起動
    eInk_print_setup();                         // E-Inkの初期化(eInk_print.ino)
    eInk_println("Example 6 PIR/Reed");         // 「Example 6 PIR/Reed」を表示
    eInk_println("BAT= " + String(batt_mv()) +" mV"); // 電池電圧をE-Inkに表示
    eInk_println("Wake= " + String(wake));      // 起動理由noをE-Inkに表示
    if(wake != ESP_SLEEP_WAKEUP_EXT0) sleep();  // ボタン以外で起動時にスリープ

    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    while(WiFi.status() != WL_CONNECTED){       // 接続に成功するまで待つ
        eInk_print(".");                        // 接続試行中表示
        if(millis() > 30000) sleep();           // 30秒超過でスリープ
        delay(500);                             // 待ち時間処理
    }
    eInk_println(WiFi.localIP());               // 本機のアドレスをE-Inkに表示
    eInk_print("-> ");                          // 矢印をE-Inkに表示
    eInk_println(UDPTO_IP);                     // UDPの宛先IPアドレスを表示
}

void loop(){                                    // 繰り返し実行する関数
    pir = digitalRead(PIN_PIR);                 // 人感センサの最新の状態を取得
    String S = String(DEVICE);                  // 送信データ保持用の文字列変数
    S += String(int(PIR ^ PIR_XOR)) + ", ";     // 起動時PIR値を送信データに追記
    S += String(int(pir ^ PIR_XOR)) + ", ";     // 起動時PIR値を送信データに追記
    S += String(batt_mv());                     // 現在の電圧値を送信データに追記
    eInk_println(S);                            // 送信データSをE-Ink表示

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
        eInk_println(url);                      // 送信URLを表示
        http.begin(url);                        // HTTPリクエスト先を設定する
        http.addHeader("Content-Type","application/x-www-form-urlencoded");
        http.addHeader("Authorization","Bearer " + String(LINE_TOKEN));
        http.POST("message=センサが反応しました。(" + S.substring(8) + ")");
        http.end();                             // HTTP通信を終了する
    }
    sleep();                                    // 下記のsleep関数を実行
}

void sleep(){                                   // スリープ実行用の関数
    delay(100);                                 // 送信完了の待ち時間処理
    WiFi.disconnect();                          // Wi-Fiの切断
    int i = 0;                                  // ループ用の数値変数i
    while(i<100){                               // スイッチ・ボタン解除待ち
        boolean pir_b = digitalRead(PIN_PIR);   // 現在の値をpir_bに保存
        if( pir == pir_b) i++;                  // 値に変化がない時はiに1を加算
        else{                                   // 変化があった時
            i = 0;                              // 繰り返し用変数iを0に
            pir = pir_b;                        // pir値を最新の値に更新
        }
        delay(1);                               // 待ち時間処理
    }
    PIR = !pir;                                 // sleep時の保存可能な変数に保持
    boolean pir_wake = 0;
    if(PIR) pir_wake = 1;                       // 次回、IOがHighのときに起動
    digitalWrite(LED_EXT_PIN, HIGH);            // LED消灯
    eInk_println("Elapsed "+String((float)millis()/1000.,1)+" Seconds");
    if(batt_mv() > 3300){                       // 電圧が3300mV以上のとき
        eInk_println("Sleeping until Ext0=" + String(pir_wake));  // 待ち表示
        /* スリープ中に GPIO12 をHighレベルに維持する */
        rtc_gpio_init(GPIO_NUM_12);
        rtc_gpio_set_direction(GPIO_NUM_12,RTC_GPIO_MODE_OUTPUT_ONLY);
        rtc_gpio_set_level(GPIO_NUM_12,1);
        // eInk_println("[!] Keep USB Pow Supply"); // 要USB電源供給表示
        // digitalWrite(POWER_HOLD_PIN, HIGH);
        esp_sleep_enable_ext0_wakeup(PIN_PIR_GPIO_NUM, pir_wake); // 割込み設定
        esp_deep_sleep_start();                 // Deep Sleepモードへ移行
    }   // else:
    eInk_println("Power OFF");                  // E-Inkへメッセージを表示
    M5.shutdown();                              // 電源OFF
}


/******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/coreink/arduino

M5Stack Arduino Library API 情報：
https://docs.m5stack.com/en/api/coreink/system_api

SB412A データシート (NANYANG SENBA OPTICAL AND ELECTRONIC CO. LTD.)

BS612 AS612 データシート (NANYANG SENBA OPTICAL AND ELECTRONIC CO. LTD.)

M5 Core Ink 回路図：
https://docs.m5stack.com/en/core/coreink
PWR_SW&ADC：GPIO12で5V DCDCと3.3V DC-DCの動作を維持できる

*******************************************************************************
【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example11_pir
https://github.com/bokunimowakaru/esp/tree/master/2_example/example43_pir
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex06_pir
*******************************************************************************/
