/*******************************************************************************
Example 16: 天気予報を表示する IoT TeleTele坊主 for M5Stack

                                          Copyright (c) 2019-2023 Wataru KUNINO
********************************************************************************

# 【重要なご注意】
# 本ソフトウェアの利用に関して、筆者(国野 亘)は、責任を負いません。
# 気象業務法や、下記の予報業務許可に関する情報、上記参考文献の注意事項を
# 良く読んでから利用ください。
# 気象業務法   https://www.jma.go.jp/jma/kishou/info/ml-17.html
# 予報業務許可 https://www.jma.go.jp/jma/kishou/minkan/q_a_m.html

地域設定： #define CITY_ID に地域コードを設定してください。
*******************************************************************************/

#include <M5Stack.h>                            // M5Stack用ライブラリの組み込み
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ

#define SSID "1234ABCD"                         // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
#define CITY_ID 130000                          // 地域コード
/*                  # 気象庁=130000(東京地方など)
                    # 大阪管区気象台=270000(大阪府など)
                    # 京都地方気象台=260000(南部など)
                    # 横浜地方気象台=140000(東部など)
                    # 銚子地方気象台=120000(北西部など)
                    # 名古屋地方気象台=230000(西部など)
                    # 福岡管区気象台=400000(福岡地方など)
*/
#define JMA_INTERVAL 3 * 3600 * 1000            // 天気予報取得間隔3時間

#define NTP_SERVER "ntp.nict.jp"                // NTPサーバのURL
#define NTP_PORT 8888                           // NTP待ち受けポート
#define NTP_INTERVAL 3 * 3600 * 1000            // NTP接続間隔 3時間
#define HOUR_SYS12 12                           // 0にすると12:00を00:00で表示

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

unsigned long TIME = 0;                         // 時計表示用の基準時刻(秒)
unsigned long TIME_ntp = 0;                     // NTPで取得したときの時刻(秒)
unsigned long TIME_alrm = 7 * 60 * 60;          // アラーム時刻(秒)
unsigned long TIME_ms = 0;                      // NTPに接続したマイコン時間(ms)
unsigned long time_ms = - NTP_INTERVAL;         // Wi-FiをONしたマイコン時間(ms)
unsigned long jma_ms = - JMA_INTERVAL;          // 天気取得したマイコン時間(ms)
String date_S = "1970/01/01";                   // 日付を保持する文字列変数
String alrm_S = "7:00";                         // アラーム時刻(文字列)
#define face_mode 4                             // 時計盤の種類を選択
boolean Alarm = false;                          // アラーム設定状態
byte setting = 0;                               // アラーム時刻設定モード
byte runApp = 0;                                // ネット機能 1:NTP, 2:LINE
    #define RUN_NTP   0x1     // NTP実行中
    #define RUN_LINE  0x2     // LINE実行中
    #define DONE_LINE 0x4     // LINE通知完了
    #define RUN_JMA   0x8     // 天気取得実行中
    
#ifndef wtr_uknw_jpg_len
    #include "jpgs/wtr_clud_jpg.h"
    #include "jpgs/wtr_fine_jpg.h"
    #include "jpgs/wtr_rain_jpg.h"
    #include "jpgs/wtr_snow_jpg.h"
    #include "jpgs/wtr_uknw_jpg.h"
#endif
const char wtrFiles[5][13]={ "wtr_uknw_jpg", "wtr_rain_jpg", "wtr_clud_jpg",
                             "wtr_fine_jpg", "wtr_snow_jpg" };

int weatherCode = 0;
char weather_s[17] = "";                        // 文字列変数を定義

int weather(){
    char s[17];
    int code = httpGetWeather(CITY_ID, s, 16);  // 天気情報を取得
    if(code != weatherCode || !strcmp(weather_s, s) ){
        weatherCode = code;
        strncpy(weather_s, s, 17);
        clock_init(4, wtrFiles[weatherCode]);   // 天気と時計を表示
        clock_showText(date_S);                 // 日付を表示
        if(Alarm) clock_showText(alrm_S, 46);   // アラーム表示
        else clock_showText(weather_s, 46);     // 天気表示
    }
    Serial.printf("DEBUG weather_s=\"%s\"(%d)\n",weather_s,strlen(weather_s) );
    return weatherCode;
}

void notify(){                                  // アラーム時刻にLINEへ通知する
    HTTPClient http;                            // HTTPリクエスト用インスタンス
    http.setConnectTimeout(15000);              // タイムアウトを15秒に設定する
    http.begin("https://notify-api.line.me/api/notify");  // HTTPリクエスト先を設定する
    http.addHeader("Content-Type","application/x-www-form-urlencoded");
    http.addHeader("Authorization","Bearer " + String(LINE_TOKEN));
    http.POST("message=アラーム(" + alrm_S  + ")が鳴りました。");
    http.end();                                 // HTTP通信を終了する
}

void ntp(){                                     // NTPで時刻を取得する
    TIME = getNtpTime(NTP_SERVER,NTP_PORT);     // NTPを用いて時刻を取得
    TIME_ntp = TIME;                            // NTP取得時刻を保持
    TIME_ms = millis();                         // NTPサーバ接続時刻を保持
}

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    M5.Speaker.setVolume(1);                    // アラーム音量を1に設定(0～10)
    M5.Lcd.setBrightness(100);                  // LCD輝度を100に
    clock_init();                               // 時計用ライブラリの起動
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
}

void loop() {                                   // 繰り返し実行する関数
    unsigned long time = TIME + (millis() - TIME_ms)/1000; // 表示する時刻
    unsigned long t_ms = (TIME % 43200)*1000 + (millis() - TIME_ms);
    t_ms %= 43200000;
    clock_Needle(t_ms);                         // 12時間制で時計の針を表示する

    if(Alarm && (TIME_alrm/60 == t_ms/60000)){  // アラーム時刻(12h制)に一致時
        if(!runApp) WiFi.begin(SSID,PASS);      // 無線LANアクセスポイント接続
        if(runApp == 0 || runApp & RUN_NTP){    // ネット機能が未設定orNTP設定時
            runApp |= RUN_LINE;                 // 接続後に Line Notify を実行
            clock_showText("to LINE", 46);      // LINE接続試行中を表示
        }                                       // (NTPよりもアラームを優先実行)
        if((t_ms/1000)%60 > 5){                 // アラーム駆動が5秒を超えた時
            if(runApp & DONE_LINE){
                Alarm = false;                  // アラームを解除
                clock_showText("", 46);
                runApp = 0x0;
            }
        }
        M5.Lcd.setBrightness(0);                // LCD輝度を0に
        // M5.Speaker.tone(880, 20);               // 20ms、音を鳴らす
        delay(50);
        M5.Lcd.setBrightness(100); delay(50);   // LCD輝度を0に
    }
    String time_S = time2str(time);             // 現在の日時を文字列に変換
    if(time_S.substring(0,10) != date_S){       // 日付が変化した時
        if(date_S.substring(0,4) != "1970"){    // 過去に受信していた時
            if(!setting) clock_init();          // 時計画面の書き直し
        }                                       // (すでに日付が書かれている為)
        date_S = time_S.substring(0,10);        // date_Sを取得した日付に更新
        clock_showText(date_S);                 // date_Sの日付を表示
        if(Alarm) clock_showText(alrm_S, 46);   // アラーム表示
    }
    if(setting && (t_ms/1000)%60 > 30){         // 設定モードで30秒以上が経過
        setting = 0;                            // 設定モードを解除
        clock_init();                           // 時計盤の再描画
        clock_showText(date_S);                 // date_Sの日付を再表示
        if(Alarm) clock_showText(alrm_S, 46);   // アラーム表示
    }
    // ボタン操作
    M5.update();                                // ボタン情報の取得
    delay(1);                                   // ボタン誤作動防止
    int adj = - M5.BtnA.isPressed() + M5.BtnC.isPressed(); // ボタン状態取得
    if(adj < 0 && setting == 0){                // 左ボタン(設定モードでない)
        adj = 0;                                // 針の操作をしない
    }
    if(adj){                                    // ボタンAかCの押下中
        TIME -= (t_ms/1000)%60;                 // 1分単位に切り捨てる
        TIME += adj * 60;                       // 60秒戻すまたは進める
        if(setting == 0){
            Alarm = false;                          // アラーム解除
            M5.Lcd.drawCircle(160,120,119,TFT_RED); // 設定中の表示(外側の円)
            M5.Lcd.drawCircle(160,120,101,TFT_RED); // 　　〃　　(内側側の円)
            clock_showText("Set Alarm", 46);        // 　　〃　　(時計盤上部)
        }
        setting++;                              // 設定状態に変更
        if(setting > 5) setting = 5;            // 長押し状態
    }else if(setting) setting = 1;              // 長押し状態の解除
    if(setting < 5) delay(100);                 // 待ち時間0.1秒
    if(M5.BtnB.wasPressed()){                   // ボタンBが押されたときに
        if(setting == 0){                       // 設定モードではない時
            Alarm = !Alarm;                     // アラーム設定を反転
        }else{                                  // 設定モードの時
            setting = 0;                        // 設定状態を解除
            TIME_alrm = t_ms/1000;              // 12時間制でアラームを設定
            Alarm = true;                       // アラーム設定
            String h = String(TIME_alrm / 3600);
            if(HOUR_SYS12 && h == "0") h = "12";
            String m = String((TIME_alrm/600)%6)+String((TIME_alrm/60)%10);
            alrm_S = h + ":" + m;
            TIME = TIME_ntp;                    // 時刻基準を取得済NTP時刻に戻す
            clock_init();                       // 時計画面の書き直し
            t_ms = (TIME % 43200)*1000 + (millis() - TIME_ms);
            clock_Needle(t_ms);                 // 12時間制で時計の針を表示する
            clock_showText(date_S);             // 日付を表示
        }
        if(Alarm){
            clock_Alarm(TIME_alrm * 1000);      // アラーム針の表示
            clock_showText(alrm_S, 46);         // アラーム表示
        }else{
            clock_AlarmOff();                   // アラーム針の消去
            clock_showText("", 46);             // アラーム表示の消去
        }
    }

    // 気象庁サーバから天気情報を取得する
    if(millis() - jma_ms > JMA_INTERVAL){       // 実行時刻になったとき
        jma_ms = millis();                      // 現在のマイコン時刻を保持
        if(!runApp) WiFi.begin(SSID,PASS);      // 無線LANアクセスポイント接続
        runApp |= RUN_JMA;
        clock_showText("to JMA", 46);           // NTP接続試行中を表示
        Serial.printf("DEBUG jma_ms=%d, app=0x%02X\n",jma_ms,runApp);
    }
    // NTPサーバから時刻情報を取得する
    if(millis() - time_ms > NTP_INTERVAL){      // NTP実行時刻になったとき
        time_ms = millis();                     // 現在のマイコン時刻を保持
        if(!runApp) WiFi.begin(SSID,PASS);      // 無線LANアクセスポイント接続
        runApp |= RUN_NTP;
        clock_showText("to NTP", 46);           // NTP接続試行中を表示
        Serial.printf("DEBUG time_ms=%d, app=0x%02X\n",time_ms,runApp);
    }

    if(WiFi.status() != WL_CONNECTED) return;   // Wi-Fi未接続のときに戻る
    clock_showText("Connected", 46);            // 接続完了表示
    if(runApp & RUN_LINE){
        notify();
        runApp &= ~RUN_LINE;
        runApp |= DONE_LINE;
    }
    if(runApp & RUN_NTP){
        ntp();
        runApp &= ~RUN_NTP;
    }
    if(runApp & RUN_JMA){
        weather();
        runApp &= ~(RUN_JMA);
    }
    WiFi.disconnect();                          // Wi-Fiの切断
    clock_showText(Alarm ? alrm_S : weather_s, 46); // 天気orアラーム表示
    Serial.printf("DEBUG weather_s=%s, app=0x%02X\n",weather_s,runApp);
    runApp &= DONE_LINE;
}

/******************************************************************************
【参考文献】Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/m5core/arduino
*******************************************************************************/

/******************************************************************************
【参考文献】M5Stack Arduino Library API 情報：
https://docs.m5stack.com/en/api/core/system
*******************************************************************************/

/*******************************************************************************
【参考文献】TFT_Clock
********************************************************************************
 An example analogue clock using a TFT LCD screen to show the time
 use of some of the drawing commands with the library.

 For a more accurate clock, it would be better to use the RTClib library.
 But this is just a demo. 
 
 This sketch uses font 4 only.

 Make sure all the display driver and pin comnenctions are correct by
 editting the User_Setup.h file in the TFT_eSPI library folder.

 #########################################################################
 ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
 #########################################################################
 
 Based on a sketch by Gilchrist 6/2/2014 1.0
 */

/*******************************************************************************
【参考文献】天気予報
天気予報 API（livedoor 天気互換サービス）, https://weather.tsukumijima.net/
天気予報 API サービスのソースコード, https://github.com/tsukumijima/weather-api
同上, https://github.com/tsukumijima/weather-api/blob/master/app/Models/Weather.php
Yahoo!天気・災害, https://weather.yahoo.co.jp/weather/rss/
Yahoo!サービスの利用規約, https://about.yahoo.co.jp/docs/info/terms/
HTTPコンテンツの連続受信方法, https://github.com/espressif/arduino-esp32/blob\
/master/libraries/HTTPClient/examples/StreamHttpClient/StreamHttpClient.ino

【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example62_weather
https://github.com/bokunimowakaru/esp/tree/master/5_learn32/esp32_25_wtr_lcd
https://github.com/bokunimowakaru/m5s/blob/master/example03_wea/example03_wea.ino
*******************************************************************************/
