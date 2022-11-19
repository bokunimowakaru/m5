/*******************************************************************************
Example 15 : Wi-Fi NTP時計 for M5Stack Core

定期的にNTPサーバから時刻情報を取得し、現在時刻を表示するアナログ風の時計です

    使用機材(例)：M5Stack Core

・中央ボタンを押すと時計盤（時刻目盛）を切り替えることが出来ます（５種類）
・左右ボタンで、長針と短針を動かすことが出来ます。（30秒後に元に戻る）
・年月日も表示します。

                                          Copyright (c) 2022 Wataru KUNINO
*******************************************************************************/

#include <M5Stack.h>                            // M5Stack用ライブラリの組み込み
#include <WiFi.h>                               // ESP32用WiFiライブラリ

#define SSID "1234ABCD"                         // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
#define NTP_SERVER "ntp.nict.jp"                // NTPサーバのURL
#define NTP_PORT 8888                           // NTP待ち受けポート
#define NTP_INTERVAL 3 * 60 * 60 * 1000         // 3時間

unsigned long TIME = 0;                         // 時計表示用の基準時刻(NTP)
unsigned long TIME_ntp = 0;                     // NTPで取得したときの時刻
unsigned long TIME_ms = 0;                      // NTPに接続したマイコン時間(ms)
unsigned long time_ms = - NTP_INTERVAL;         // Wi-FiをONしたマイコン時間(ms)
String date_S = "1970/01/01";                   // 日付を保持する文字列変数
byte face_mode;                                 // 時計盤の種類を選択
byte setting = 0;                               // 設定モード

void setup(){                                   // 一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    face_mode = clock_init();                   // 時計用ライブラリの起動
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
}

void loop() {                                   // 繰り返し実行する関数
    unsigned long t_ms = (TIME % 43200)*1000 + ((millis()-TIME_ms)%43200000);
    clock_Needle(t_ms);                         // 時計の針を表示する
    String S = time2str(TIME + (millis() - TIME_ms)/1000); // 日時を取得
    if(S.substring(0,10) != date_S){            // 日付が変化した時
        if(date_S.substring(0,4) != "1970"){    // 過去に受信していた時
            if(!setting) clock_init();          // 時計画面の書き直し
        }                                       // (すでに日付が書かれている為)
        date_S = S.substring(0,10);             // date_Sを取得した日付に更新
        clock_showText(date_S);                 // date_Sの日付を表示
    }
    if(setting && (t_ms/1000)%60 > 30){
        clock_init();
        clock_showText(date_S);                 // date_Sの日付を表示
        setting = 0;
    }
    // ボタン操作
    M5.update();                                // ボタン情報の取得
    delay(1);                                   // ボタン誤作動防止
    int adj = - M5.BtnA.isPressed() + M5.BtnC.isPressed(); // ボタン状態取得
    if(adj){                                    // ボタンAかCの押下中
        TIME -= (t_ms/1000)%60;                 // 1分単位に切り捨てる
        TIME += adj * 60;                       // 60秒戻すまたは進める
        setting++;                              // 設定状態に変更
        if(setting > 5) setting = 5;            // 長押し状態
        M5.Lcd.drawCircle(160,120,119,TFT_RED); // 設定中の表示(赤サークル)
        M5.Lcd.drawCircle(160,120,101,TFT_RED); // 設定中の表示(赤サークル)
    }else if(setting) setting = 1;              // 長押し状態の解除
    if(setting < 5) delay(100);                 // 待ち時間0.1秒
    if(M5.BtnB.wasPressed()){                   // ボタンBが押されたときに
        if(setting){                            // 設定状態の時
            setting = 0;                        // 設定状態を解除
            TIME = TIME_ntp;                    // 時刻基準を取得済NTP時刻に戻す
            clock_init();                       // 時計画面の書き直し
        }else{                                  // 設定状態ではなかったとき
            face_mode = clock_init(face_mode + 1); // 時計盤の種類を変更
        }
        clock_showText(date_S);                 // 日付を表示
    }
    // NTPサーバから時刻情報を取得する
    if(millis() - time_ms > NTP_INTERVAL){      // NTP実行時刻になったとき
        time_ms = millis();                     // 現在のマイコン時刻を保持
        WiFi.begin(SSID,PASS);                  // 無線LANアクセスポイント接続
        M5.Lcd.drawString("Wi-Fi ON",271,0,1);  // 無線LAN起動表示
    }
    if(WiFi.status() != WL_CONNECTED) return;   // Wi-Fi未接続のときに戻る
    M5.Lcd.fillRect(265,0,56,8,TFT_BLACK);      // 接続表示の消去
    M5.Lcd.drawString("Connected",265,0,1);     // 接続表示
    TIME = getNtpTime(NTP_SERVER,NTP_PORT);     // NTPを用いて時刻を取得
    TIME_ntp = TIME;                            // NTP取得時刻を保持
    TIME_ms = millis();                         // NTPサーバ接続時刻を保持
    WiFi.disconnect();                          // Wi-Fiの切断
    M5.Lcd.fillRect(265,0,56,8,TFT_BLACK);      // 接続表示の消去
}
