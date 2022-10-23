/*******************************************************************************
Example 9 : ESP32C3 Wi-Fi コンシェルジェ アナウンス担当（音声合成出力）
                                                    for ESP32 / ATOM / ATOM Lite

AquosTalkを使った音声合成でユーザへ気づきを通知することが可能なIoT機器です。

    対応IC： AquosTalk Pico LSI

    AquosTalk接続用
    TXD -> AquosTalk Pico LSI側 RXD端子(2番ピン)

    使用機材(例)： ESP32 / ATOM / ATOM Lite + AquosTalk Pico LSI
    
                                          Copyright (c) 2016-2022 Wataru KUNINO
*******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/atom/arduino

ATOM Lite Arduino Library API 情報(本サンプルでは使用しない)：
https://docs.m5stack.com/en/api/atom/system

【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example21_talk
https://github.com/bokunimowakaru/esp/tree/master/2_example/example53_talk
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex09_talk
*******************************************************************************/

#include <WiFi.h>                           // ESP32用WiFiライブラリ
#include <WebServer.h>                      // HTTPサーバ用ライブラリ

#define PIN_LED_RGB 27                      // IO27 に WS2812を接続(Atom内蔵)
#define PIN_SS2_RX 32                       // シリアル受信ポート(未使用)
#define PIN_SS2_TX 26                       // シリアル送信 AquosTalk Pico LSI側

#define SSID "1234ABCD"                     // 無線LANアクセスポイントのSSID
#define PASS "password"                     // パスワード
#define PORT 1024                           // 受信ポート番号

HardwareSerial serial2(2);                  // シリアル2を生成
WebServer server(80);                       // Webサーバ(ポート80=HTTP)定義

void handleRoot(){
    char talk[97] = "de-ta'o'nyu-ryo_kushiteku'dasai."; // 音声出力用の文字列
    Serial.println("Connected");            // 接続されたことを表示
    if(server.hasArg("TEXT")){              // 引数TEXTが含まれていた時
        String rx = server.arg("TEXT");     // 引数TEXTの値を取得し変数rxへ代入
        rx.toCharArray(talk,97);
        trUri2txt(talk);                    // URLエンコードの変換処理
    }
    if(server.hasArg("VAL")){               // 引数VALが含まれていた時
        int i = server.arg("VAL").toInt();  // 引数VALの値を取得し変数rxへ代入
        snprintf(talk,96,"su'-tiwa <NUMK VAL=%d>desu.",i);
    }
    if(strlen(talk) > 0){                   // 文字列が代入されていた場合、
        led(40,0,0);                        // (WS2812)LEDを赤色で点灯
        serial2.print("\r$");               // ブレークコマンドを出力する
        delay(100);                         // 待ち時間処理
        serial2.print(talk);                // 受信文字データを音声出力
        serial2.print('\r');                // 改行コード（CR）を出力する
        led(0,20,0);                        // (WS2812)LEDを緑色で点灯
    }
    String tx = getHtml(talk);              // HTMLコンテンツを取得
    server.send(200, "text/html", tx);      // HTMLコンテンツを送信
}

void setup(){                               // 起動時に一度だけ実行する関数
    led_setup(PIN_LED_RGB);                 // WS2812の初期設定(ポート設定)
    Serial.begin(115200);                   // 動作確認のためのシリアル出力開始
    Serial.println("ESP32 eg.9 talk");      // タイトルをシリアル出力表示
    serial2.begin(9600, SERIAL_8N1, PIN_SS2_RX, PIN_SS2_TX); // シリアル初期化
    serial2.print("\r$");                   // ブレークコマンドを出力する
    delay(100);                             // 待ち時間処理
    serial2.print("$?kon'nnichi/wa.\r");    // 音声「こんにちわ」を出力する
    WiFi.mode(WIFI_STA);                    // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                  // 無線LANアクセスポイントへ接続
    while(WiFi.status() != WL_CONNECTED){   // 接続に成功するまで待つ
        led((millis()/50) % 10);            // (WS2812)LEDの点滅
        delay(50);                          // 待ち時間処理
    }
    Serial.print(WiFi.localIP());           // IPアドレスを表示
    serial2.print("<NUM VAL=");             // 数字読み上げ用タグ出力
    serial2.print(WiFi.localIP());          // IPアドレスを読み上げる
    serial2.print(">.\r");                  // タグの終了を出力する
    server.on("/", handleRoot);             // HTTP接続時のコールバック先を設定
    server.begin();                         // Web サーバを起動する
    led(0,20,0);                            // (WS2812)LEDを緑色で点灯
}

void loop(){                                // 繰り返し実行する関数
    server.handleClient();                  // クライアントからWebサーバ呼出
}
