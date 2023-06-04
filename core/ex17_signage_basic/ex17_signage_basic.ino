/*******************************************************************************
Example 17 : Wi-Fi デジタル・サイネージ for M5Stack Core デジタル時計 Basic版

・定期的(1分ごと)にHTTPサーバから画像を取得し、LCDに表示します。
・また、NTPサーバから時刻情報を取得し、時計を表示します。

    使用機材(例)：M5Stack Core、Raspberry Pi、インターネット接続環境

【機能】
・インターネット上のNTPサーバから時刻を自動取得します。
・アナログ風の時計を表示します。
・画像保持用に疑似SRAMを使用します(確保できなかった場合はSPIFFS FLASHを使用)

【準備】
・HTTPサーバが必要です。#define BMP_SERVERに設定してください。
・初期状態ではGitHub上の画像を取得します。#define BMP_SERVER が重複しているので
　動作確認後にhttpsから始まる方の行を削除してください。
・Raspberry Pi用のHTTPサーバはtoolsフォルダ内の signage_serv.py にあります。
　cd ~/m5/tools⏎ ./signage_serv.py⏎ を実行してHTTPサーバを起動してください。
・送信する画像はtools内のhtmlフォルダに保存してあります。

【操作方法】
・左ボタンを押すと、JPEG画像を取得して時刻情報とともに表示します。
・中央ボタンで、2値BMP画像を取得して時刻情報とともに表示します。
・右ボタンで、時計表示のOFF/ON切り替えを行います。

【参考文献】
・本ファイルの末尾に記載します。

【その他】
・M5StackのLCDは僅か2インチで、輝度も室内向けです。
　（表題「デジタル・サイネージ」のような使い方は限定的にしかできません。）

                                          Copyright (c) 2023 Wataru KUNINO
*******************************************************************************/

#include <M5Stack.h>                            // M5Stack用ライブラリの組み込み
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiClientSecure.h>                   // TLS(SSL)通信用ライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ
#include <SPIFFS.h>

#define SSID "1234ABCD"                         // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
#define BMP_SERVER "http://192.168.1.2:8080/"   // LAN上のコンテンツ・サーバのURL
#define BMP_SERVER "https://raw.githubusercontent.com/bokunimowakaru/m5/master/tools/html/"
#define BMP_PSRAM_SIZE 32768                    // BMPファイルの最大サイズ32KB
#define NTP_SERVER "ntp.nict.jp"                // NTPサーバのURL
#define NTP_PORT 8888                           // NTP待ち受けポート
#define NTP_INTERVAL 3 * 3600 * 1000            // 3時間

String files[] = {"photo.jpg","mono.bmp"};      // ファイル名
int file_num = 0;                               // ファイル番号
boolean clock_en = true;                        // 時計表示
unsigned long TIME = 0;                         // 時計表示用の基準時刻(秒)
unsigned long TIME_ntp = 0;                     // NTPで取得したときの時刻(秒)
unsigned long TIME_ms = 0;                      // NTPに接続したマイコン時刻(ms)
unsigned long time_ms = - NTP_INTERVAL;         // Wi-FiをONしたマイコン時刻(ms)
String date_S = "1970/01/01,00:00";             // 日付を保持する文字列変数
byte runApp = 0;                                // ネット機能 1:NTP, 2:LINE
    #define RUN_NTP   1     // NTP実行中
    #define RUN_GET   2     // HTTP GET実行中

void httpget(){                                 // コンテンツ(画像)を取得する
    File file;                                  // SRAMが無かった時のFLASH用
    file = SPIFFS.open("/tmp","w");             // SPIFFSを使用
    if(file==0){
        Serial.printf("ERROR: SPIFFS\n");
        return;                                 // ファイルを開けれなければ戻る
    }
    WiFiClientSecure client;                    // TLS/TCP/IP接続部の実体を生成
    client.setInsecure(); // 証明書を確認しない // client.setCACert(証明書); 
    HTTPClient http;                            // HTTPリクエスト用インスタンス
    http.setConnectTimeout(10000);              // タイムアウトを10秒に設定する
    http.begin(String(BMP_SERVER)+files[file_num]);  // HTTPリクエスト先を設定
    int code = http.GET();
    if(code != 200 || http.getSize() <= 0){
        Serial.println( \
            "ERROR HTTP Code="+String(code) + \
            ", Size="+String(http.getSize()) \
        );
        disp_text("HTTP "+String(code));
        return;
    }
        
    byte buf[1024];                             // 1Kバイトの受信バッファ作成
    int len = 0;
    WiFiClient *stream = http.getStreamPtr();
    int read_n = stream->available();
    while(stream && read_n > 0 && len < BMP_PSRAM_SIZE ){
        if(read_n > 1024) read_n = 1024; else delay(100);
        if(len + read_n > BMP_PSRAM_SIZE) read_n = BMP_PSRAM_SIZE - len;
        stream->readBytes(buf, read_n);
        file.write(buf,read_n);
        len += read_n;
        read_n = stream->available();
        // Serial.printf("len=%d, read_n=%d\n",len,read_n);
    }
    Serial.printf("Loaded %d Bytes\n",len);
    http.end();                                 // HTTP通信を終了する
    client.stop();                              // TLS(SSL)通信の停止
    file.close();                               // ファイルを閉じる
    
    // 取得したコンテンツの表示
    if(files[file_num].endsWith(".jpg")){
        M5.Lcd.drawJpgFile(SPIFFS, "/tmp",0,0);
    }else if(files[file_num].endsWith(".bmp")){
        drawMonoBitmapFile("/tmp",clock_en ? 128 : 255);
    }
    
    // 時計表示
    if(clock_en){
        M5.Lcd.setTextColor(TFT_BLACK);
        M5.Lcd.drawCentreString(date_S.substring(0,10),160,80,4);
        M5.Lcd.drawCentreString(date_S.substring(11,16),160,160,4);
    }
    return;                                     // ファイルを開けれなければ戻る
}

void ntp(){                                     // NTPで時刻を取得する
    TIME = getNtpTime(NTP_SERVER,NTP_PORT);     // NTPを用いて時刻を取得
    TIME_ntp = TIME;                            // NTP取得時刻を保持
    TIME_ms = millis();                         // NTPサーバ接続時刻を保持
}

void disp_text(String S){
    M5.Lcd.fillRect(120,120,80,15,TFT_WHITE);
    M5.Lcd.drawCentreString(S,160,120,2);
}

void setup(){                                   // 一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    M5.Lcd.setBrightness(100);                  // LCD輝度を100に
    if(!SPIFFS.begin()){                        // SDカード失敗時に SPIFFS開始
        M5.Lcd.println("Formating FS");         // SPIFFS未フォーマット時の表示
        SPIFFS.format();                        // ファイル全消去
        M5.Lcd.println( SPIFFS.begin() ? "OK" : "ERROR"); delay(3000);
    }
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
}

void loop() {                                   // 繰り返し実行する関数
    unsigned long time = TIME + (millis() - TIME_ms)/1000; // 表示する時刻
    unsigned long t_ms = (TIME % 43200)*1000 + (millis() - TIME_ms);
    t_ms %= 43200000;

    String time_S = time2str(time);             // 現在の日時を文字列に変換
    if(time_S.substring(0,16) != date_S){       // 日時が変化した時
        date_S = time_S.substring(0,16);        // date_Sを取得した日付に更新
        if(!runApp) WiFi.begin(SSID,PASS);      // 無線LANアクセスポイント接続
        runApp |= RUN_GET;
    }
    
    M5.update();                                // ボタン情報の取得
    int btn = M5.BtnA.wasPressed() + 2 * M5.BtnB.wasPressed()\
                                   + 3 * M5.BtnC.wasPressed();
    if(btn > 0 && btn <= 3){
        switch(btn){
            case 1:
                file_num = 0;
                disp_text("Get JPEG");
                break;
            case 2:
                file_num = 1;
                disp_text("Get BMP");
                break;
            case 3:
            default:
                clock_en = !clock_en;
                char off_on[2][4] = {"OFF","ON"};
                disp_text("Clock "+String(off_on[clock_en]));
        }
        if(!runApp) WiFi.begin(SSID,PASS);      // 無線LANアクセスポイント接続
        runApp |= RUN_GET;
    }
    delay(100);
    
    // NTPサーバから時刻情報を取得する
    if(millis() - time_ms > NTP_INTERVAL){      // NTP実行時刻になったとき
        time_ms = millis();                     // 現在のマイコン時刻を保持
        if(!runApp) WiFi.begin(SSID,PASS);      // 無線LANアクセスポイント接続
        runApp |= RUN_NTP;
        runApp |= RUN_GET;
        disp_text("to NTP");                    // NTP接続試行中を表示
    }
    
    if(WiFi.status() != WL_CONNECTED) return;   // Wi-Fi未接続のときに戻る
    disp_text("Connected");                     // 接続完了表示
    if(runApp & RUN_NTP){
        ntp();
        runApp &= ~RUN_NTP;
    }
    if(runApp & RUN_GET){
        httpget();
        runApp &= ~RUN_GET;
    }
    if(!runApp){
        WiFi.disconnect();                      // Wi-Fiの切断
    }
}

/******************************************************************************
【参考文献】Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/core2/arduino
*******************************************************************************/

/******************************************************************************
【参考文献】M5Stack Arduino Library API 情報：
https://docs.m5stack.com/en/api/core2/system
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
 
/******************************************************************************
【参考文献】Example 15 : Wi-Fi NTP時計 for M5Stack Core2：
https://github.com/bokunimowakaru/m5/tree/master/core2/ex15_clock
*******************************************************************************/
