/*******************************************************************************
Example 7: GPS(GNSS)の位置情報を取得し、Wi-Fiで送信する
・動作確認済みGPSモジュール：u-blox NEO-6M NEO-6M-0-001, 杭州中科微电子 AT6558

    使用機材(例)：M5Stack Core + Mini GPS/BDS Unit (AT6558)

                                               Copyright (c) 2022 Wataru KUNINO
********************************************************************************
★ご注意★・GPS Unitの電源を入れてから位置情報が得られるまで数分以上を要します
*******************************************************************************/

#include <M5Stack.h>                            // M5Stack用ライブラリの組み込み

#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ
#include "lib_TinyGPS.h"                        // GPS通信用ライブラリ

#define SSID "1234ABCD"                         // 無線LANアクセスポイント SSID
#define PASS "password"                         // パスワード
#define PORT 1024                               // 送信のポート番号
#define DEVICE "gns_s_3,"                       // デバイス名(5字+"_"+番号+",")
#define LOGUDP "log_s_0,"                       // デバイス名(5字+"_"+番号+",")
RTC_DATA_ATTR int mode = 0;                     // 0:日本地図 1:TEXT 2:Raw
String mode_S[3] = {"Map","Text","Raw"};

/******************************************************************************
 Ambient 設定
 ******************************************************************************
 ※Ambientのアカウント登録と、チャネルID、ライトキーの取得が必要です。
    1. https://ambidata.io/ へアクセス
    2. 右上の[ユーザ登録(無料)]ボタンでメールアドレス、パスワードを設定して
       アカウントを登録
    3. [チャネルを作る]ボタンでチャネルIDを新規作成する
    4. 「チャネルID」を下記のAmb_Idのダブルコート(")内に貼り付ける
    5. 「ライトキー」を下記のAmb_Keyに貼り付ける
   (参考文献)
    IoTデータ可視化サービスAmbient(アンビエントデーター社) https://ambidata.io/
*******************************************************************************/
#define Amb_Id  "00000"                         // AmbientのチャネルID
#define Amb_Key "0000000000000000"              // Ambientのライトキー

/******************************************************************************
 UDP 宛先 IP アドレス設定
 ******************************************************************************
 カンマ区切りでUPD宛先IPアドレスを設定してください。
 末尾を255にすると接続ネットワーク(アクセスポイント)にブロードキャスト
 *****************************************************************************/
IPAddress UDPTO_IP = {255,255,255,255};         // UDP宛先 IPアドレス

TinyGPS gps;                                    // GPSライブラリのインスタンス
float lat=0., lon=0., alt=0.;                   // 緯度・経度・標高
unsigned short sat=0;
unsigned long hdop=0, age=0;
boolean trig = false;                           // 送信用トリガ
unsigned long base_ms = 0;                      // センサ検知時の時刻

#define LCD_ORIGIN_Y 16                         // 表示開始位置
#define LCD_LINES   (240 - LCD_ORIGIN_Y)/8      // 表示可能な行数

int lcd_line = 0;                               // 現在の表示位置(行)
char lcd_buf[LCD_LINES][54];                    // LCDスクロール表示用バッファ

void lcd_log(String S){                         // LCDにログを表示する
    lcd_line++;                                 // 次の行に繰り上げる
    if(lcd_line >= LCD_LINES){                  // 行末を超えたとき
        lcd_line = LCD_LINES - 1;               // 行末に戻す
        for(int y=1;y<LCD_LINES;y++){           // 1行分、上にスクロールする処理
            int org_y = 8*(y-1)+LCD_ORIGIN_Y;   // 文字を表示する位置を算出
            M5.Lcd.fillRect(0, org_y, 320, 8, BLACK);
            M5.Lcd.setCursor(0, org_y);
            M5.Lcd.print(lcd_buf[y]);           // バッファ内のデータを表示する
            memcpy(lcd_buf[y-1],lcd_buf[y],54); // バッファ内データを1行分移動
        }
    }
    M5.Lcd.fillRect(0, 8*lcd_line+LCD_ORIGIN_Y, 320, 8, BLACK);
    M5.Lcd.setCursor(0, 8*lcd_line+LCD_ORIGIN_Y); // 現在の文字表示位置に移動
    M5.Lcd.print(S.substring(0,53));            // 53文字までを表示
    S.toCharArray(lcd_buf[lcd_line],54);        // 配列型変換(最大長時\0付与有)
}

String ip2s(uint32_t ip){                       // IPアドレスを文字列に変換
    String S;
    for(int i=3;i>=0;i--){
        S += String((ip>>(8*ip))%256);
        if(i) S += ".";
    }
    return S;
}

void lcd_cls(){                                 // LCDを消去する関数
    M5.Lcd.fillScreen(BLACK);                   // 表示内容を消去
    M5.Lcd.setCursor(0,0);                      // 文字表示位置を0,0に
    M5.Lcd.print(" M5 GNSS/GPS ");              // 「GNSS/GPS」をLCD表示
    M5.Lcd.print(mode_S[mode] + " ");           // 起動操作をLCD表示
    M5.Lcd.println(WiFi.localIP());             // 本機のアドレスをLCD表示
    lcd_line = 0;                               // 現在の表示位置を保持
}

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    M5.Lcd.setBrightness(31);                   // 輝度を下げる（省エネ化）
    M5.Lcd.drawJpgFile(SD, "/japan.jpg");       // LCDに日本地図japan.jpgを表示
    setupGps();                                 // GPS初期化
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
}

void loop(){                                    // 繰り返し実行する関数
    M5.update();                                // ボタン状態の取得
    int btn=M5.BtnA.wasPressed()+2*M5.BtnB.wasPressed()+4*M5.BtnC.wasPressed();
    switch(btn){
        case 1: mode = 0; break;                // 0:日本地図
        case 2: mode = 1; break;                // 1:TEXT
        case 4: mode = 2; break;                // 2:Raw + UDP送信
        default: btn = 0; break;
    }

    if(mode == 1){                              // 地図表示モード
        getGpsPos(gps,&lat,&lon,&alt,&sat,&hdop,&age);  // GPS情報取得
        lcd_log("GPS: " + String(sat) + " satellites");         // ログをLCD表示
    }

    if(mode == 2){                              // ダブルクリック時
        char s[128];                            // 最大文字長127文字
        boolean i = getGpsRawData(s,128);       // GPS生データを取り込む
        if(s[0] != 0){
            Serial.println(s);                  // データがあるときは表示
            for(int i=0;i<2;i++) if(strstr(s,gps.GPS_TERM_NAMES[i])){
                lcd_log(s);                     // 位置情報のときにLCDに表示
            }
            if(WiFi.status() == WL_CONNECTED){
                WiFiUDP udp;                    // UDP通信用のインスタンス定義
                udp.beginPacket(UDPTO_IP, PORT);  // UDP送信先を設定
                udp.print(LOGUDP);              // ログ用ヘッダを送信
                udp.println(s);                 // 値を送信
                udp.endPacket();                // UDP送信の終了(実際に送信)
            }
        }
    }

    if(!trig && millis() - base_ms > 30000){    // 30秒以上経過
        base_ms = millis();
        trig = true;
        if(WiFi.status() != WL_CONNECTED){
            WiFi.begin(SSID,PASS);                  // 無線LANアクセスポイント接続
        }
    }

    // lcd_log("Wi-Fi: STAT = " + String(WiFi.status()));

    if(trig && WiFi.status() == WL_CONNECTED){
        getGpsPos(gps,&lat,&lon,&alt,&sat,&hdop,&age);  // GPSから位置情報を取得
        if(alt>1000) alt = 0.;                      // 測定不可時の対応
        String S = String(DEVICE);                  // 送信データ保持用の文字列変数
        S += String(lat,6) + ", ";                  // 緯度値を送信データに追記
        S += String(lon,6) + ", ";                  // 経度値を送信データに追記
        S += String(alt,0) + ", ";                  // 標高値を送信データに追記
        S += String(sat) + ", ";                    // 衛生数を送信データに追記
        S += String(hdop) + ", ";                   // DHOP値を送信データに追記
        S += String(age);                           // 取得経過時間を追記
        lcd_log("GPS: " + S);                       // LCDに表示

        WiFiUDP udp;                                // UDP通信用のインスタンス定義
        udp.beginPacket(UDPTO_IP, PORT);            // UDP送信先を設定
        udp.println(S);                             // センサ値を送信
        udp.endPacket();                            // UDP送信の終了(実際に送信)
        delay(10);                                  // 送信待ち時間

        if(strcmp(Amb_Id,"00000") != 0){            // Ambient設定時
            S = "{\"writeKey\":\""+String(Amb_Key); // (項目)writeKey,(値)ライトキー
            S+= "\",\"d1\":\"" + String(alt,0);     // (項目)d1,(値)標高値
            S+= "\",\"lat\":\"" + String(lat,6);    // (項目)lat,(値)緯度値
            S+= "\",\"lng\":\"" + String(lon,6);    // (項目)lng,(値)経度値
            S+= "\"}";
            HTTPClient http;                        // HTTPリクエスト用インスタンス
            http.setConnectTimeout(15000);          // タイムアウトを15秒に設定する
            String url="http://ambidata.io/api/v2/channels/"+String(Amb_Id)+"/data";
            http.begin(url);                        // HTTPリクエスト先を設定する
            http.addHeader("Content-Type","application/json"); // JSON形式を設定する
            lcd_log(url);                           // 送信URLを表示
            http.POST(S);                           // GPS情報をAmbientへ送信する
            http.end();                             // HTTP通信を終了する
        }
        if(mode != 2) WiFi.disconnect();            // Wi-Fiの切断
        trig = false;
    }
}

/*******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/m5stickc_plus/arduino

M5StickC Arduino Library API 情報 (旧モデル M5StackC 用)：
https://docs.m5stack.com/en/api/stickc/system_m5stickc

【引用コード】
https://github.com/bokunimowakaru/SORACOM-LoRaWAN/blob/master/examples/cqp_ex05_gps_bin
*******************************************************************************/