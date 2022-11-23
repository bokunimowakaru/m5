/*******************************************************************************
Example 15 : Wi-Fi NTP時計 for M5Stack Core

定期的にNTPサーバから時刻情報を取得し、現在時刻を表示するアナログ風の時計です

    使用機材(例)：M5Stack Core

【機能】
・インターネット上のNTPサーバから時刻を自動取得します。
・アナログ風の時計を表示します。
・年月日も表示します。
・アラーム(12時間制)の設定が出来ます。
・アラーム時にLINEへ通知することが出来ます。

【操作方法】
・左ボタンを押すと時計盤（時刻目盛）を切り替えることが出来ます（５種類）
・中央ボタンでアラームの入り／切り設定を行います。
・右ボタンで、アラーム時刻の設定モードに入ります。
　設定モードでは、左右のボタンで長針と短針を早送りし、中央ボタンで確定します。
　（操作せずに30秒が経過すると元に戻ります。）

【ご注意１】12時間制の表記方法について
・本ソフトウェアでは、正午と深夜0時から１時間を12:00～12:59 と表示します。
　（一般的なデジタル時計と同じ表示方式です。）
・NHKなどで使用する様式に合わせたい場合は HOUR_SYS12 を0にしてください。
　(参考文献：NHKことばのハンドブック：「午後0:XX」は「12:XX」とは言わない)

【ご注意２】時計の画像について
時計画面の一部に illustimage.com からダウンロードしたコンテンツを改変したものが
含まれているので、有料商品に利用にする場合は、権利者に確認してください。
ダウンロード時の権利情報は同フォルダの jpegsREADME.txt に記載の通りです。

【参考文献】
本ファイルの末尾に記載します。

                                          Copyright (c) 2022 Wataru KUNINO
*******************************************************************************/

#include <M5Stack.h>                            // M5Stack用ライブラリの組み込み
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ

#define SSID "1234ABCD"                         // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
#define NTP_SERVER "ntp.nict.jp"                // NTPサーバのURL
#define NTP_PORT 8888                           // NTP待ち受けポート
#define NTP_INTERVAL 3 * 3600 * 1000            // 3時間
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
String date_S = "1970/01/01";                   // 日付を保持する文字列変数
String alrm_S = "7:00";                         // アラーム時刻(文字列)
byte face_mode = 2;                             // 時計盤の種類を選択
boolean Alarm = false;                          // アラーム設定状態
byte setting = 0;                               // アラーム時刻設定モード
byte runApp = 0;                                // ネット機能 1:NTP, 2:LINE
    #define RUN_NTP   1     // NTP実行中
    #define RUN_LINE  2     // LINE実行中
    #define DONE_LINE 3     // LINE通知完了

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

void setup(){                                   // 一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    M5.Speaker.setVolume(1);                    // アラーム音量を1に設定(0～10)
    M5.Lcd.setBrightness(100);                  // LCD輝度を100に
    face_mode = clock_init();                   // 時計用ライブラリの起動
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
}

void loop() {                                   // 繰り返し実行する関数
    unsigned long time = TIME + (millis() - TIME_ms)/1000; // 表示する時刻
    unsigned long t_ms = (TIME % 43200)*1000 + (millis() - TIME_ms);
    t_ms %= 43200000;
    clock_Needle(t_ms);                         // 12時間制で時計の針を表示する

    if(Alarm && (TIME_alrm/60 == t_ms/60000)){  // アラーム時刻(12h制)に一致時
        if(!runApp) WiFi.begin(SSID,PASS);      // 無線LANアクセスポイント接続
        if(runApp == 0 || runApp == RUN_NTP){   // ネット機能が未設定orNTP設定時
            runApp = RUN_LINE;                  // 接続後に Line Notify を実行
            clock_showText("to LINE", 46);      // LINE接続試行中を表示
        }                                       // (NTPよりもアラームを優先実行)
        if((t_ms/1000)%60 > 5){                 // アラーム駆動が5秒を超えた時
            if(runApp == DONE_LINE){
                Alarm = false;                  // アラームを解除
                clock_showText("", 46);
                runApp = 0;
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
        face_mode = clock_init(face_mode + 1);  // 時計盤の種類を変更
        clock_showText(date_S);                 // 日付を表示
        if(Alarm) clock_showText(alrm_S, 46);   // アラーム表示
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
    // NTPサーバから時刻情報を取得する
    if(millis() - time_ms > NTP_INTERVAL){      // NTP実行時刻になったとき
        time_ms = millis();                     // 現在のマイコン時刻を保持
        if(!runApp) WiFi.begin(SSID,PASS);      // 無線LANアクセスポイント接続
        runApp = RUN_NTP;
        clock_showText("to NTP", 46);           // NTP接続試行中を表示
    }

    if(WiFi.status() != WL_CONNECTED) return;   // Wi-Fi未接続のときに戻る
    clock_showText("Connected", 46);            // 接続完了表示
    if(runApp == RUN_LINE){
        notify();
        runApp = DONE_LINE;
    }
    if(runApp == RUN_NTP){
        ntp();
        runApp = 0;
    }
    WiFi.disconnect();                          // Wi-Fiの切断
    clock_showText(Alarm ? alrm_S : "", 46);    // アラーム表示
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
