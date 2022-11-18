/*******************************************************************************
Example 15 : Wi-Fi NTP時計 for M5Stack Core

定期的にNTPサーバから時刻情報を取得し、現在時刻を表示するアナログ風の時計です

    使用機材(例)：M5Stack Core

                                          Copyright (c) 2022 Wataru KUNINO
*******************************************************************************/

#include <M5Stack.h>                            // M5Stack用ライブラリの組み込み
#include <WiFi.h>                               // ESP32用WiFiライブラリ

#define SSID "1234ABCD"                         // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
#define NTP_SERVER "ntp.nict.jp"                // NTPサーバのURL
#define NTP_PORT 8888                           // NTP待ち受けポート
#define NTP_INTERVAL 3 * 60 * 60 * 1000         // 3時間

unsigned long TIME = 0;                         // NTPで取得した時刻
unsigned long TIME_ms = 0;                      // NTPに接続したマイコン時間(ms)
unsigned long time_ms = - NTP_INTERVAL;         // Wi-FiをONしたマイコン時間(ms)

void setup(){                                   // 一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    clock_init();                               // 時計用ライブラリの起動
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
}

void loop() {                                   // 繰り返し実行する関数
    // 時計の針を表示する
    clock_Needle( (TIME % 43200)*1000 + ((millis()-TIME_ms)%43200000) );
    // 時刻を文字列で表示する
    M5.Lcd.drawString(time2str(TIME + (millis() - TIME_ms)/1000),0,0,1);
    // NTPサーバから時刻情報を取得する
    if(millis() - time_ms > NTP_INTERVAL){      // NTP実行時刻になったとき
        time_ms = millis();                     // 現在のマイコン時刻を保持
        WiFi.begin(SSID,PASS);                  // 無線LANアクセスポイント接続
        M5.Lcd.drawString("Wi-Fi ON",271,0,1);  // 無線LAN起動表示
    }
    delay(100);
    if(WiFi.status() != WL_CONNECTED) return;   // Wi-Fi未接続のときに戻る
    M5.Lcd.drawString("Connected",265,0,1);     // 接続表示
    TIME = getNtpTime(NTP_SERVER,NTP_PORT);     // NTPを用いて時刻を取得
    TIME_ms = millis();                         // NTPサーバ接続時刻を保持
    WiFi.disconnect();                          // Wi-Fiの切断
    M5.Lcd.fillRect(265,0,56,8,TFT_BLACK);      // 接続表示の消去
}
