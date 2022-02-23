/*******************************************************************************
Example 7: 現在のGPS(GNSS)の位置情報を送信する
・GPSモジュール：u-blox NEO-6M NEO-6M-0-001
・ボタン長押し起動で連続動作します。

                                               Copyright (c) 2022 Wataru KUNINO
********************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/m5stickc_plus/arduino

M5StickC Arduino Library API 情報 (旧モデル M5StackC 用)：
https://docs.m5stack.com/en/api/stickc/system_m5stickc

【引用コード】
https://github.com/bokunimowakaru/SORACOM-LoRaWAN/blob/master/examples/cqp_ex05_gps_bin
*******************************************************************************/

#include <M5StickCPlus.h>                       // M5StickC Plus 用ライブラリ

#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ
#include "esp_sleep.h"                          // ESP32用Deep Sleep ライブラリ
#include "lib_TinyGPS.h"

#define PIN_BTN 37                              // G37 に 操作ボタン
#define PIN_BTN_GPIO_NUM GPIO_NUM_37            // G37 をスリープ解除信号へ設定
#define SSID "1234ABCD"                         // 無線LANアクセスポイント SSID
#define PASS "password"                         // パスワード
#define PORT 1024                               // 送信のポート番号
#define SLEEP_P 30*1000000ul                    // スリープ時間 30秒(uint32_t)
#define DEVICE "gns_s_5,"                       // デバイス名(5字+"_"+番号+",")
#define LOGUDP "log_s_0,"                       // デバイス名(5字+"_"+番号+",")
#define GPS_TIMEOUT_MS 6*1000                   // GPS測定の時間制限
#define WIFI_TIMEOUT_MS GPS_TIMEOUT_MS+10*1000  // Wi-Fi接続の時間制限

/******************************************************************************
 UDP 宛先 IP アドレス設定
 ******************************************************************************
 カンマ区切りでUPD宛先IPアドレスを設定してください。
 末尾を255にすると接続ネットワーク(アクセスポイント)にブロードキャスト
 *****************************************************************************/
IPAddress UDPTO_IP = {255,255,255,255};         // UDP宛先 IPアドレス

int wake = (int)esp_sleep_get_wakeup_cause();   // 起動理由を変数wakeに保存
int clickType = 0;                              // 操作:1=Norm,2=Double,3=Long
String btn_S[]={"No","Single","Double","Long"}; // ボタン名

TinyGPS gps;                                    // GPSライブラリのインスタンスを定義
float lat=0., lon=0., alt=0.;                   // 緯度・経度・標高
unsigned short sat=0;
unsigned long hdop=0, age=0;

struct LoRaPayload{                             // LoRaWAN送信用の変数(11バイト以下)
  uint8_t gpsHeader=0x21;                       // GPS_HEADER(uint8型 1バイト)
  int32_t lat=0;                                // 緯度(int32型 4バイト) 1,000,000倍
  int32_t lon=0;                                // 経度(int32型 4バイト) 1,000,000倍
  int16_t alt=0;                                // 標高(int16型 2バイト) 単位＝ｍ
};

int get_clickType(){                            // ボタン操作内容を取得する
    int btn_pre = (int)!digitalRead(PIN_BTN);   // ボタンの初期状態を取得
    while(millis() < 1000){                     // 1000msの判定期間
        if(digitalRead(PIN_BTN)){               // ボタン状態が開放の時
            if(btn_pre == 1) btn_pre = 2;       // クリック押下→開放を保持
        }else{                                  // ボタン状態が押下の時
            if(btn_pre != 1) return 2;          // ダブルクリックと判定
        }
    }
    if(btn_pre == 1) return 3;                  // ロング・プレスと判定
    return 1;                                   // シングル・クリックと判定
}

// M5Stick C Plus LCD 240 x 135 (40 x 17) フォント8x6ピクセル
int lcd_line = 0;
char lcd_buf[16][41];

void lcd_log(String S){
    lcd_line++;
    if(lcd_line>16){
        lcd_line = 16;
        M5.Lcd.setCursor(0,8);
        for(int y=1;y<16;y++){
            M5.Lcd.fillRect(6*strlen(lcd_buf[y]), 8*y, 240, 8, BLACK);
            M5.Lcd.println(lcd_buf[y]);
            memcpy(lcd_buf[y-1],lcd_buf[y],41);
        }
    }
    M5.Lcd.setCursor(0, 8*lcd_line);
    M5.Lcd.print(S);
    S.toCharArray(lcd_buf[lcd_line-1],41);
}

unsigned long lcd_time = millis();
void lcd_log_full(String S){
    lcd_line++;
    if(lcd_line>16){
        lcd_line = 16;
    }
    if(millis() > lcd_time + 300){
        if(lcd_line != 1) M5.Lcd.fillRect(0, 8*lcd_line, 240, 8, BLACK);
        lcd_line = 1;
    }
    int len = 6 * S.length();
    M5.Lcd.fillRect(len, 8*lcd_line, 240 - len, 8, BLACK);
    M5.Lcd.setCursor(0, 8*lcd_line);
    M5.Lcd.print(S);
    lcd_time = millis();
}

void lcd_head(){
    M5.Lcd.setCursor(0,0);
    M5.Lcd.fillRect(0, 0, 240, 8, BLACK);
    M5.Lcd.print(" M5 GNSS/GPS ");              // 「GNSS」をLCD表示
    /*
    switch(wake){
        case 0: M5.Lcd.print("Pow "); break;
        case ESP_SLEEP_WAKEUP_EXT0: M5.Lcd.print("BTN "); break;
        case ESP_SLEEP_WAKEUP_TIMER: M5.Lcd.print("TIMER "); break;
        default: M5.Lcd.print("w"+String(wake)+" "); break;
    }
    */
    M5.Lcd.print(btn_S[clickType] + " ");       // 起動操作をLCD表示
    M5.Lcd.println(WiFi.localIP());             // 本機のアドレスをLCD表示
    lcd_line = 0;
}

void lcd_cls(){
    M5.Lcd.fillScreen(BLACK);                   // LCDを消去
    lcd_head();
    memset(lcd_buf,0,16*41);
}

void setup(){                                   // 起動時に一度だけ実行する関数
    pinMode(PIN_BTN,INPUT_PULLUP);              // ボタン入力の設定
    pinMode(M5_LED,OUTPUT);                     // LEDのIOを出力に設定
    digitalWrite(M5_LED,LOW);                   // LED ON
    if(wake == ESP_SLEEP_WAKEUP_EXT0) clickType=1;  // ボタンで起動
    clickType = get_clickType();                // ボタン操作内容を取得
    while(!digitalRead(PIN_BTN));               // 操作完了待ち
    delay(100);                                 // チャタリング防止用
    digitalWrite(M5_LED,HIGH);                  // LED OFF

    WiFi.mode(WIFI_OFF);                        // 無線LANをOFFモードに設定
    M5.begin();                                 // M5Stack用ライブラリの起動
    M5.Axp.ScreenBreath(7+2);                   // LCDの輝度を2に設定
    M5.Lcd.setRotation(1);                      // LCDを横向き表示に設定
    M5.Lcd.setTextFont(1);                      // 8x6ピクセルのフォント
    lcd_cls();                                  // LCD消去
    setupGps();                                 // GPS初期化

    if(clickType == 2){                         // ダブルクリック時
        WiFi.mode(WIFI_STA);                    // 無線LANをSTAモードに設定
        WiFi.begin(SSID,PASS);                  // 無線LANアクセスポイント接続
        while(digitalRead(PIN_BTN)){            // ボタン未押下状態でモニタ
            char s[128];                        // 最大文字長127文字
            boolean i = getGpsRawData(s,128);   // GPS生データを取り込む
            if(s[0] != 0){
                Serial.println(s);              // データがあるときは表示
                for(int i=0;i<2;i++) if(strstr(s,gps.GPS_TERM_NAMES[i])){
                    lcd_log(s);                 // 位置情報のときにLCDに表示
                }
                if(WiFi.status() == WL_CONNECTED){
                    WiFiUDP udp;                      // UDP通信用のインスタンス定義
                    udp.beginPacket(UDPTO_IP, PORT);  // UDP送信先を設定
                    udp.print(LOGUDP);                // ログ用ヘッダを送信
                    udp.println(s);                   // 値を送信
                    udp.endPacket();                  // UDP送信の終了(実際に送信)
                }
            }
        }
    }

    boolean i=1,res=0;
    while(clickType <= 1 && !res){              // 通常起動かつGPSデータ未取得時
        res = getGpsPos(gps,&lat,&lon,&alt,&sat,&hdop,&age);    // GPSデータ取得
        lcd_log("GPS: " + String(sat) + " satellites");
        digitalWrite(M5_LED,(i^=1));            // LEDの点滅で動作表示
        if(millis() > GPS_TIMEOUT_MS || !digitalRead(PIN_BTN)) sleep();
    }

    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイント接続
    while(WiFi.status() != WL_CONNECTED){       // 接続に成功するまで待つ
        if(i) lcd_log("Wi-Fi: STAT = " + String(WiFi.status()));
        digitalWrite(M5_LED,(i^=1));            // LEDの点滅で動作表示
        if(millis() > WIFI_TIMEOUT_MS) sleep(); // 時間超過でスリープ
        delay(500);                             // 待ち時間処理
    }
    lcd_log("Wi-Fi: " + String(WiFi.localIP()) + " -(UDP)-> " + String(UDPTO_IP));
}

void loop(){                                    // 繰り返し実行する関数
    LoRaPayload data;                           // 送信用の構造体変数dataを定義

    if(getGpsPos(gps,&lat,&lon,&alt,&sat,&hdop,&age)){      // GPSから位置情報を取得
        data.lat=(int32_t)(lat*1.e6);   // 緯度を構造体変数dataへ保存
        data.lon=(int32_t)(lon*1.e6);   // 経度を構造体変数dataへ保存
        data.alt=(int16_t)(alt);        // 標高を構造体変数dataへ保存
        Serial.print("lat = ");         // 緯度をシリアル出力表示
        Serial.print(data.lat);         // ±180 000 000
        Serial.print(", lon = ");       // 経度をシリアル出力表示
        Serial.print(data.lon);         // ±180 000 000
        Serial.print(", alt = ");       // 標高をシリアル出力表示
        Serial.println(data.alt);       // 単位＝ m メートル
        delay(100);                     // シリアル出力の完了待ち
    }

    String S = String(DEVICE);              // 送信データ保持用の文字列変数
    S += String(int(data.lat)) + ", ";      // 緯度値を送信データに追記
    S += String(int(data.lon)) + ", ";      // 経度値を送信データに追記
    S += String(int(data.alt)) + ", ";      // 標高値を送信データに追記
    S += String(sat) + ", ";                // 衛生数を送信データに追記
    S += String(hdop) + ", ";               // DHOP値を送信データに追記
    S += String(age);                       // 取得経過時間を送信データに追記
    lcd_log("GPS: " + S);                       // LCDに表示

    WiFiUDP udp;                                // UDP通信用のインスタンス定義
    udp.beginPacket(UDPTO_IP, PORT);            // UDP送信先を設定
    udp.println(S);                             // センサ値を送信
    udp.endPacket();                            // UDP送信の終了(実際に送信)
    delay(10);                                  // 送信待ち時間
/*
    HTTPClient http;                            // HTTPリクエスト用インスタンス
    http.setConnectTimeout(15000);              // タイムアウトを15秒に設定する
    String url;                                 // URLを格納する変数を生成
*/
    if(digitalRead(PIN_BTN)==0) clickType = 0;  // ボタン押下時にclickTypeを0に
    if(clickType <= 1) sleep();                 // sleep関数を実行
}

void sleep(){                                   // スリープ実行用の関数
    WiFi.disconnect();                          // Wi-Fiの切断
    digitalWrite(M5_LED,LOW);                   // LED ON
    lcd_cls();
    lcd_log(" Btn  = " + String(digitalRead(PIN_BTN)));  // ボタン状態をLCDに表示
    int i = 0;                                  // ループ用の数値変数i
    while(i<100){                               // スイッチ・ボタン解除待ち
        i = digitalRead(PIN_BTN) ? i+1 : 0;     // ボタン開放時にiに1を加算
        delay(1);                               // 待ち時間処理
    }
    digitalWrite(M5_LED,HIGH);                  // LED OFF
    lcd_log("Sleep...");                        // 「Sleep」をシリアル出力表示
    delay(100);                                 // 待ち時間処理
    esp_sleep_enable_ext0_wakeup(PIN_BTN_GPIO_NUM,0);   // 割込み設定
    esp_deep_sleep(SLEEP_P);                    // Deep Sleepモードへ移行
}
