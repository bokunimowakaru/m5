/*******************************************************************************
Example 3: Wi-Fi 照度計 for M5 ATOM S3
・照度センサ から取得した照度値を送信するIoTセンサです。

    使用機材(例)：M5 ATOM S3 + ATOM-HAT(ATOM-MATEに付属) + HAT-DLIGHT

                                          Copyright (c) 2021-2023 Wataru KUNINO
*******************************************************************************/

#include "M5AtomS3.h"                           // ATOM S3 用ライブラリ
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ

#define SSID "1234ABCD"                         // 無線LANアクセスポイントSSID
#define PASS "password"                         // パスワード

#define PORT 1024                               // 送信のポート番号
#define SLEEP_P 30*1000000ul                    // スリープ時間 30秒(uint32_t)
#define DEVICE "illum_5,"                       // デバイス名(5字+"_"+番号+",")

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

void setup(){                                   // 起動時に一度だけ実行する関数
    bh1750Setup(6,5);                           // 照度センサの初期化
    M5.begin();                                 // M5StickC用Lcdライブラリの起動
//  M5.Lcd.setBrightness(200);                  // LCDの輝度設定(#if 0で未定義)
    analogMeterInit("lx", "Illum", 0, 1000);    // アナログ・メータの初期表示
    M5.Lcd.setTextColor(WHITE);                 // 文字色を白に
    M5.Lcd.setCursor(0,72);                     // 文字の表示位置を原点に設定
    M5.Lcd.print("ex.03 Illum.");               // タイトル表示
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
}

void loop(){                                    // 繰り返し実行する関数
    M5.update();                                // ボタン状態の取得
    if(M5.Btn.wasPressed()){
        M5.update(); 
        while(!M5.Btn.wasPressed())M5.update(); 
    }
    if(millis()%(SLEEP_P/1000) == 0){           // SLEEP_P間隔で下記を実行
        WiFi.begin(SSID,PASS);                  // 無線LANアクセスポイント接続
    }
    if(millis()%500) return;                    // 以下は500msに1回だけ実行する

    M5.Lcd.fillRect(91, 72, 37, 8, BLACK);      // Wi-Fi接続の待ち時間
    M5.Lcd.setCursor(91, 72);                   // 文字位置を設定
    M5.Lcd.printf("(%d) ",WiFi.status());       // Wi-Fi状態番号を表示
    M5.Lcd.print((SLEEP_P/1000 - millis()%(SLEEP_P/1000))/1000);
    
    float lux = getLux();                       // 照度(lux)を取得
    if(lux < 0.) return;                        // 取得失敗時にloopの先頭に戻る
    analogMeterNeedle(lux,5);                   // 照度に応じてメータ針を設定
    
    if(lux > 1000 * 3 / 4){                     // メータ値が3/4以上のとき
        M5.Lcd.fillRect(0,88, 128,15,TFT_RED);  // 表示部の背景を塗る
    }else{
        M5.Lcd.fillRect(0,88, 128,15, BLACK);   // 表示部の背景を塗る
    }

    String S = "Illum.= " + String(lux,0);      // 照度値を文字列変数Sに代入
    S += " lx";                                 // 単位を追記
    M5.Lcd.drawCentreString(S, 64, 88, 2);      // 文字列を表示
    if(WiFi.status() != WL_CONNECTED) return;   // Wi-Fi未接続のときに戻る

    M5.Lcd.fillRect(0, 80, 128, 8, BLACK);      // 表示部の背景を塗る
    M5.Lcd.setCursor(0, 80);                    // 文字位置を設定
    M5.Lcd.println(WiFi.localIP());             // 本機のアドレスをシリアル出力

    S = String(DEVICE) + String(lux,0);         // 送信データSにデバイス名を代入
    Serial.println(S);                          // 送信データSをシリアル出力表示
    WiFiUDP udp;                                // UDP通信用のインスタンスを定義
    udp.beginPacket(UDPTO_IP, PORT);            // UDP送信先を設定
    udp.println(S);                             // 送信データSをUDP送信
    udp.endPacket();                            // UDP送信の終了(実際に送信する)
    
    if(strcmp(Amb_Id,"00000") != 0){            // Ambient設定時に以下を実行
        S = "{\"writeKey\":\""+String(Amb_Key); // (項目)writeKey,(値)ライトキー
        S += "\",\"d1\":\"" + String(lux);      // (項目)d1,(値)照度
        S += "\"}";
        HTTPClient http;                        // HTTPリクエスト用インスタンス
        http.setConnectTimeout(15000);          // タイムアウトを15秒に設定する
        String url = "http://ambidata.io/api/v2/channels/"+String(Amb_Id)+"/data";
        http.begin(url);                        // HTTPリクエスト先を設定する
        http.addHeader("Content-Type","application/json"); // JSON形式を設定する
        Serial.println(url);                    // 送信URLを表示
        http.POST(S);                           // センサ値をAmbientへ送信する
        http.end();                             // HTTP通信を終了する
    }
    delay(100);                                 // 送信完了の待ち時間処理
    WiFi.disconnect();                          // Wi-Fiの切断
}

/*
void sleep(){                                   // スリープ実行用の関数
    delay(100);                                 // 送信完了の待ち時間処理
    WiFi.disconnect();                          // Wi-Fiの切断
//  M5.Lcd.setBrightness(0);                    // LCDの輝度設定(#if 0で未定義)
    M5.Lcd.fillScreen(BLACK);                   // LCDの消去
//  M5.Lcd.sleep();                             // LCDの電源オフ(#if 0で未定義)
    Serial.println("Sleep...");                 // 「Sleep」をシリアル出力表示
//  esp_sleep_enable_ext0_wakeup(GPIO_NUM_41,0); // ボタン割込み設定(G41)
    esp_deep_sleep(SLEEP_P);                    // Deep Sleepモードへ移行
}
*/

/*******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/atoms3/arduino

M5StickC Arduino Library API 情報 (M5StackC 用 ※ATOM S3用ではない )：
https://docs.m5stack.com/en/api/stickc/system_m5stickc

BH1750FVI データシート 2011.11 - Rev.D (ローム)

M5Atom S3用ライブラリ(GitHub)：
https://github.com/m5stack/M5AtomS3

【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example06_lum
https://github.com/bokunimowakaru/esp/tree/master/2_example/example38_lum
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex03_lum
*******************************************************************************/
