/*******************************************************************************
Example 1: Wi-Fi コンシェルジェ 照明担当 for ATOM / ATOM Lite
・HTTPによるWebサーバ機能搭載 Wi-FiコンシェルジェがLEDを制御します。

                                          Copyright (c) 2021-2022 Wataru KUNINO
*******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/atom/arduino

ATOM Lite Arduino Library API 情報(本サンプルでは使用しない)：
https://docs.m5stack.com/en/api/atom/system

【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example16_led
https://github.com/bokunimowakaru/esp/tree/master/2_example/example48_led
*******************************************************************************/

#include <WiFi.h>                           // ESP32用WiFiライブラリ
#include <WebServer.h>                      // HTTPサーバ用ライブラリ

#define PIN_LED_RGB 27                      // 内蔵 RGB LED
#define SSID "1234ABCD"                     // 無線LANアクセスポイントのSSID
#define PASS "password"                     // パスワード

WebServer server(80);                       // Webサーバ(ポート80=HTTP)定義
int led_stat = 0;                           // LED状態用の変数led_statを定義

void handleRoot(){
    String rx, tx;                          // 受信用,送信用文字列

    if(server.hasArg("L")){                 // 引数Lが含まれていた時
        rx = server.arg("L");               // 引数Lの値を取得し変数rxへ代入
        led_stat = rx.toInt();              // 変数sから数値を取得しled_statへ
    }
    tx = getHtml(led_stat);                 // HTMLコンテンツを取得
    server.send(200, "text/html", tx);      // HTMLコンテンツを送信

    Serial.println(led_stat);               // LED状態led_stat値を表示
    if(abs(led_stat) >= 1){                 // led_statの絶対値が1以上の時
        led(20);                            // RGB LEDを点灯(輝度20)
    }else{
        led(0);                             // RGB LEDを消灯
    }
}

void setup(){                               // 起動時に一度だけ実行する関数
    led_setup(PIN_LED_RGB);                 // RGB LED の初期設定(ポートを設定)
    Serial.begin(115200);                   // 動作確認用シリアル出力開始
    Serial.println("M5 LED HTTP");          // 「LED HTTP」をシリアル出力表示
    WiFi.mode(WIFI_STA);                    // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                  // 無線LANアクセスポイントへ接続
    while(WiFi.status() != WL_CONNECTED){   // 接続に成功するまで待つ
        led((millis()/50) % 10);            // RGB LEDの点滅
        delay(50);                          // 待ち時間処理
    }
    morseIp0(-1,50,WiFi.localIP());         // IPアドレス終値をモールス信号出力
    server.on("/", handleRoot);             // HTTP接続時のコールバック先を設定
    server.begin();                         // Web サーバを起動する
    Serial.println(WiFi.localIP());         // 本機のIPアドレスをシリアル出力
}

void loop(){                                // 繰り返し実行する関数
    server.handleClient();                  // クライアントからWebサーバ呼び出し
}
