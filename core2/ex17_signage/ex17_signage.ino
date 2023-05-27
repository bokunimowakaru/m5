/*******************************************************************************
Example 17 : Wi-Fi デジタル・サイネージ for M5Stack Core2

・定期的にHTTPサーバから画像を取得し、LCDに表示します。
・また、NTPサーバから時刻情報を取得し、時計を表示します。

    使用機材(例)：M5Stack Core2

【機能】
・インターネット上のNTPサーバから時刻を自動取得します。
・アナログ風の時計を表示します。
・画像保持用に疑似SRAMを使用します(確保できなかった場合はSPIFFS FLASHを使用)

【準備】
・HTTPサーバが必要です。
・HTTPサーバのサンプルはtoolsフォルダ内の signage_serv.py にあります。
・送信する画像はtools内のhtmlフォルダに保存してください。

【操作方法】
・左ボタンを押すと####################W
・中央ボタンで#############。
・右ボタンで、############。

【参考文献】
本ファイルの末尾に記載します。

                                          Copyright (c) 2023 Wataru KUNINO
*******************************************************************************/

#include <M5Core2.h>                            // M5Stack用ライブラリの組み込み
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiClientSecure.h>                   // TLS(SSL)通信用ライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ
#include <SPIFFS.h>

#define SSID "1234ABCD"                         // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
//#define BMP_SERVER "http://192.168.1.2:8080/" // コンテンツ・サーバのURL
#define BMP_SERVER \
"https://raw.githubusercontent.com/bokunimowakaru/m5/master/tools/html/"
#define BMP_INTERVAL 3 * 60 * 1000              // コンテンツ取得間隔 3分
#define BMP_PSRAM_SIZE 1048576                  // 1MBの疑似SRAMを確保
#define NTP_SERVER "ntp.nict.jp"                // NTPサーバのURL
#define NTP_PORT 8888                           // NTP待ち受けポート
#define NTP_INTERVAL 3 * 3600 * 1000            // 3時間

String files[] = {"photo.jpg","mono.bmp","mono.bmp"}; // ファイル名
int file_num = 0;                               // ファイル番号
boolean clock_en = true;                        // 時計表示
unsigned long TIME = 0;                         // 時計表示用の基準時刻(秒)
unsigned long TIME_ntp = 0;                     // NTPで取得したときの時刻(秒)
unsigned long TIME_ms = 0;                      // NTPに接続したマイコン時刻(ms)
unsigned long time_ms = - NTP_INTERVAL;         // Wi-FiをONしたマイコン時刻(ms)
unsigned long bmp_ms = - BMP_INTERVAL;          // 画像取得用 Wi-Fi ON 時刻(ms)
String date_S = "1970/01/01";                   // 日付を保持する文字列変数
byte runApp = 0;                                // ネット機能 1:NTP, 2:LINE
    #define RUN_NTP   1     // NTP実行中
    #define RUN_GET   2     // HTTP GET実行中

void httpget(){                                 // コンテンツ(画像)を取得する
    byte *psram = NULL;                         // 疑似SRAM用
    File file;                                  // SRAMが無かった時のFLASH用
    if(psramInit()){
        psram = (byte *) ps_malloc(BMP_PSRAM_SIZE); // 1MBの疑似SRAMを確保
    }
    if(!psram){                                 // 確保できなかった場合
        file = SPIFFS.open("/tmp","w");         // SPIFFSを使用
        if(file==0) return;                     // ファイルを開けれなければ戻る
    }
    WiFiClientSecure client;                    // TLS/TCP/IP接続部の実体を生成
    client.setInsecure(); // 証明書を確認しない // client.setCACert(証明書); 
    HTTPClient http;                            // HTTPリクエスト用インスタンス
    http.setConnectTimeout(5000);               // タイムアウトを5秒に設定する
    http.begin(String(BMP_SERVER)+files[file_num]);  // HTTPリクエスト先を設定
    if(http.GET() == 200 && http.getSize() > 0){
        byte buf[1024];                         // 1Kバイトの受信バッファ作成
        int len = 0;
        WiFiClient *stream = http.getStreamPtr();
        int read_n = stream->available();
        while(stream && read_n > 0 && len < BMP_PSRAM_SIZE ){
            if(read_n > 1024) read_n = 1024; else delay(100);
            if(len + read_n > BMP_PSRAM_SIZE) read_n = BMP_PSRAM_SIZE - len;
            stream->readBytes(buf, read_n);
            if(psram) memcpy(psram+len,buf,read_n); else file.write(buf,read_n);
            len += read_n;
            read_n = stream->available();
            // Serial.printf("len=%d, read_n=%d\n",len,read_n);
        }
        Serial.printf("Loaded %d Bytes\n",len);
        if(psram){
            if(files[file_num].endsWith(".jpg")){
                M5.Lcd.drawJpg(psram, len);
            }else if(files[file_num].endsWith(".bmp")){
                drawMonoBitmap(psram, 320, 240,clock_en ? 128 : 255);
            }
        }else{
            file.close();                       // ファイルを閉じる
            if(files[file_num].endsWith(".jpg")){
                M5.Lcd.drawJpgFile(SPIFFS, "/tmp",0,0);
            }else if(files[file_num].endsWith(".bmp")){
                drawMonoBitmapFile("/tmp",clock_en ? 128 : 255);
            }
        }
        if(clock_en) clock_init(-1);            // 壁紙を維持して時計を再描画
    }else Serial.println("ERROR HTTP "+String(http.GET()));
    http.end();                                 // HTTP通信を終了する
    client.stop();                              // TLS(SSL)通信の停止
    file.close();                               // ファイルを閉じる
    free(psram);                                // 疑似SRAMを開放する
}

void ntp(){                                     // NTPで時刻を取得する
    TIME = getNtpTime(NTP_SERVER,NTP_PORT);     // NTPを用いて時刻を取得
    TIME_ntp = TIME;                            // NTP取得時刻を保持
    TIME_ms = millis();                         // NTPサーバ接続時刻を保持
}

void setup(){                                   // 一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    M5.Lcd.setBrightness(100);                  // LCD輝度を100に
    if(!SPIFFS.begin()){                        // SDカード失敗時に SPIFFS開始
        M5.Lcd.println("Formating FS");         // SPIFFS未フォーマット時の表示
        SPIFFS.format();                        // ファイル全消去
        M5.Lcd.println( SPIFFS.begin() ? "OK" : "ERROR"); delay(3000);
    }
    clock_init();                               // 時計用ライブラリの起動
    M5.Spk.begin();                             // Initialize the speaker.
    M5.Spk.DingDong();                          // Play the DingDong sound.
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
}

void loop() {                                   // 繰り返し実行する関数
    unsigned long time = TIME + (millis() - TIME_ms)/1000; // 表示する時刻
    unsigned long t_ms = (TIME % 43200)*1000 + (millis() - TIME_ms);
    t_ms %= 43200000;
    if(clock_en) clock_Needle(t_ms);            // 12時間制で時計の針を表示する

    String time_S = time2str(time);             // 現在の日時を文字列に変換
    if(time_S.substring(0,10) != date_S){       // 日付が変化した時
        date_S = time_S.substring(0,10);        // date_Sを取得した日付に更新
        clock_showText(date_S);                 // date_Sの日付を表示
    }
    
    // ボタン操作時にサーバから画像を取得
    M5.update();                                // ボタン情報の取得
    delay(100);                                 // 待ち時間0.1秒
    int btn = M5.BtnA.wasPressed() + 2 * M5.BtnB.wasPressed()\
                                   + 3 * M5.BtnC.wasPressed();
    if(btn > 0 && btn <= 3){
        M5.Spk.DingDong();                      // Play the DingDong sound.
        file_num = btn - 1; 
        switch(btn){
            case 1:
                clock_en = true;
                clock_showText("Get JPEG");
                break;
            case 2:
                clock_en = true;
                clock_showText("Get BMP");
                break;
            case 3:
            default:
                clock_en = false;
                M5.Lcd.fillRect(120,150,80,15,TFT_WHITE);
                M5.Lcd.drawCentreString("Clock OFF",160,150,2);
        }
        if(!runApp) WiFi.begin(SSID,PASS);      // 無線LANアクセスポイント接続
        runApp |= RUN_GET;
    }
    
    // NTPサーバから時刻情報を取得する
    if(millis() - time_ms > NTP_INTERVAL){      // NTP実行時刻になったとき
        time_ms = millis();                     // 現在のマイコン時刻を保持
        if(!runApp) WiFi.begin(SSID,PASS);      // 無線LANアクセスポイント接続
        runApp |= RUN_NTP;
        runApp |= RUN_GET;
        if(clock_en)clock_showText("to NTP");   // NTP接続試行中を表示
    }
    
    // BMPサーバから画像データを取得する
    if(millis() - bmp_ms > BMP_INTERVAL){       // 画像取得時刻になったとき
        bmp_ms = millis();                      // 現在のマイコン時刻を保持
        if(!runApp) WiFi.begin(SSID,PASS);      // 無線LANアクセスポイント接続
        runApp |= RUN_GET;
        if(clock_en)clock_showText("to HtServ"); // HTTPサーバ接続試行中を表示
    }

    if(WiFi.status() != WL_CONNECTED) return;   // Wi-Fi未接続のときに戻る
    if(clock_en) clock_showText("Connected");   // 接続完了表示
    if(runApp & RUN_GET){
        httpget();
        runApp &= ~RUN_GET;
    }
    if(runApp & RUN_NTP){
        ntp();
        runApp &= ~RUN_NTP;
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

/*******************************************************************************
【参考文献】speak
********************************************************************************
*******************************************************************************
* Copyright (c) 2021 by M5Stack
*                  Equipped with M5Core2 sample source code
*                          配套  M5Core2 示例源代码
* Visit for more information: https://docs.m5stack.com/en/core/core2
* 获取更多资料请访问: https://docs.m5stack.com/zh_CN/core/core2
*
* Describe: Speaker example.  喇叭示例
* Date: 2022/7/26
*******************************************************************************
*/
 