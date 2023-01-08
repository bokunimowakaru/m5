/*******************************************************************************
Example 4: Wi-Fi コンシェルジェ 掲示板担当
                                                    for ESP32 / ATOM / ATOM Lite
・各種IoTセンサが送信したデータをLCDに表示します。
・HTTPによるWebサーバ機能搭載 Wi-FiコンシェルジェがLCDに文字を表示します。

    LCD接続用：
    IO19 AE-AQM0802 SDAポート 
    IO22 AE-AQM0802 SCLポート 設定方法＝lcdSetup(8桁,2行,SDA,SCL)

    使用機材(例)：ESP32 / ATOM / ATOM Lite + SeeedStudio Grove Serial Camera Kit
                  + LCD AE-AQM1602A(KIT) or AE-AQM0802A

                                          Copyright (c) 2016-2022 Wataru KUNINO
*******************************************************************************/

#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#include <WebServer.h>                          // HTTPサーバ用ライブラリ

#define PIN_LED_RGB 27                          // M5Atom 内蔵 RGB LED
#define SSID "1234ABCD"                         // 無線LANアクセスポイントSSID
#define PASS "password"                         // パスワード
#define PORT 1024                               // 受信ポート番号
#define TIMEOUT 20000                           // タイムアウト 20秒
#define LCD_WIDTH 16                            // LCD 桁数 AQM1602は16、0802は8
#define LCD_SDA 19                              // LCD SDA接続用 GPIOポート番号
#define LCD_SCL 22                              // LCD SCL接続用 GPIOポート番号

WebServer server(80);                           // Webサーバ(ポート80=HTTP)定義

void handleRoot(){
    String rx, tx;                              // 受信用,送信用文字列
    led(20,0,0);                                // (WS2812)LEDを赤色に変更
    if(server.hasArg("TEXT")){                  // クエリTEXTが含まれていた時
        rx = server.arg("TEXT").substring(0,96); // クエリ値を文字変数rxへ代入
    }
    tx = getHtml(rx);                           // HTMLコンテンツを取得
    server.send(200, "text/html", tx);          // HTMLコンテンツを送信
    lcdPrint(rx);
    led(0,20,0);                                // (WS2812)LEDを緑色に戻す
}

WiFiUDP udp;                                    // UDP通信用のインスタンスを定義

void setup(){                                   // 起動時に一度だけ実行する関数
    led_setup(PIN_LED_RGB);                     // WS2812の初期設定(ポート設定)
    lcdSetup(LCD_WIDTH, 2, LCD_SDA, LCD_SCL);   // LCD初期化(X=8,Y=2,SDA,SCL)
    Serial.begin(115200);                       // 動作確認用のシリアル出力開始
    Serial.println("Example 04 lcd");           // 「Example 04」をシリアル出力
    lcdPrint("ESP32 eg.04 LCD");                // 「Example 04」をLCDに表示する

    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    while(WiFi.status() != WL_CONNECTED){       // 接続に成功するまで待つ
        led((millis()/50) % 10);                // WS2812の点滅
        delay(50);                              // 待ち時間処理
    }
    led(0,20,0);                                // (WS2812)LEDを緑色で点灯
    server.on("/", handleRoot);                 // HTTP接続時コールバック先設定
    server.begin();                             // Web サーバを起動する
    lcdPrintIp2(WiFi.localIP());                // IPアドレスを液晶の2行目に表示
    udp.begin(PORT);                            // UDP通信御開始
}

void loop(){                                    // 繰り返し実行する関数
    server.handleClient();                      // クライアントからWebサーバ呼出
    char lcd[97];                               // LCD表示用文字列変数 97バイト
    memset(lcd, 0, 97);                         // 文字列変数lcd初期化(97バイト)
    int len = udp.parsePacket();                // 受信パケット長を変数lenに代入
    if(len==0)return;                           // 未受信のときはloop()の先頭に
    led(20,0,0);                                // (WS2812)LEDを赤色に変更
    udp.read(lcd, 96);                          // 受信データを文字列変数lcdへ
    udp.flush();                                // 受信できなかったデータを破棄
    Serial.print(lcd);                          // シリアルへ出力する
    lcdPrint(lcd);                              // 受信文字データを液晶へ表示
    led(0,20,0);                                // (WS2812)LEDを緑色に戻す
}

/******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/atom/arduino

ATOM Lite Arduino Library API 情報(本サンプルでは使用しない)：
https://docs.m5stack.com/en/api/atom/system

【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example18_lcd
https://github.com/bokunimowakaru/esp/tree/master/2_example/example50_lcd
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex04_lcd
*******************************************************************************/
