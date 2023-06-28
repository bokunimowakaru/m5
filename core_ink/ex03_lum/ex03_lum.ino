/*******************************************************************************
Example 3: ESP32 (IoTセンサ) Wi-Fi 照度計 for M5Stack CORE INK
・照度センサ から取得した照度値を送信するIoTセンサです。

    使用機材(例)：M5Stack CORE INK + HAT-DLIGHT
        HAT 設定方法＝shtSetup(25,26);

    DLIGHT HAT の代わりに DLIGHT UNIT (Grove互換端子)を使用するときは
    ポート番号の変更が必要です。
        UNIT設定方法＝shtSetup(32,33);
    
                                          Copyright (c) 2021-2023 Wataru KUNINO
*******************************************************************************/

#include <M5CoreInk.h>                          // M5Stack用ライブラリ組み込み
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
    注意事項:SLEEP_Pを28.8秒以下にしないこと(※必ず 30*1000000ul 以上に設定)
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

int batt_mv(){                                  // 電池電圧確認
    int PIN_AIN = 35;                           // 電池電圧取得用のADCポート
    float adc;                                  // ADC値の代入用
    analogSetAttenuation(ADC_2_5db);            // ADC 0.1V～1.25V入力用
    pinMode(PIN_AIN, ANALOG);                   // GPIO35をアナログ入力に
    adc = analogReadMilliVolts(PIN_AIN);        // AD変換器から値を取得
    adc /= 5.1 / (20 + 5.1);                    // 抵抗分圧の逆数
    return (int)(adc + 0.5);                    // 電圧値(mV)を整数で応答
}

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    bh1750Setup(25,26);                         // 照度センサの初期化
    
    ink_print_setup();                          // Inkの初期化(ink_print.ino)
    ink_println("Example 3 LUM");               // 「Example 3 LUM」を表示
    ink_println("BAT= " + String(batt_mv()) +" mV"); // 電池電圧をInkに表示
}

void loop(){                                    // 繰り返し実行する関数
    ink_print("...", false);                    // ...をInk用バッファへ
    while(WiFi.status() != WL_CONNECTED){       // 接続に成功するまで待つ
        if(millis() > 30000) sleep();           // 30秒超過でスリープ
        if(millis()%500 == 0) ink_print(".");   // 接続試行中表示
    }
    ink_println(WiFi.localIP());                // 本機のアドレスをInkに表示
    ink_print("-> ", false);                    // 矢印をInk用バッファへ
    ink_println(UDPTO_IP);                      // UDPの宛先IPアドレスを表示

    float lux = getLux();                       // 照度(lux)を取得
    int batt = batt_mv();                       // 電池電圧を取得してbattに代入
    if(lux < 0.) sleep();                       // 取得失敗時に末尾のsleepを実行

    String S = String(DEVICE) + String(lux,0);  // 送信データSにデバイス名を代入
    S += ", " + String(batt);                   // 変数battの値を追記
    ink_println(S);                             // 送信データSをInk表示
    
    WiFiUDP udp;                                // UDP通信用のインスタンスを定義
    udp.beginPacket(UDPTO_IP, PORT);            // UDP送信先を設定
    udp.println(S);                             // 送信データSをUDP送信
    udp.endPacket();                            // UDP送信の終了(実際に送信する)
    if(strcmp(Amb_Id,"00000") == 0) sleep();    // Ambient未設定時にsleepを実行

    S = "{\"writeKey\":\""+String(Amb_Key);     // (項目)writeKey,(値)ライトキー
    S += "\",\"d1\":\"" + String(lux);          // (項目)d1,(値)照度
    S += "\",\"d2\":\"" + String(batt) + "\"}"; // (項目)d2,(値)電圧

    HTTPClient http;                            // HTTPリクエスト用インスタンス
    http.setConnectTimeout(15000);              // タイムアウトを15秒に設定する
    String url = "http://ambidata.io/api/v2/channels/"+String(Amb_Id)+"/data";
    http.begin(url);                            // HTTPリクエスト先を設定する
    http.addHeader("Content-Type","application/json"); // JSON形式を設定する
    int code = http.POST(S);                    // センサ値をAmbientへ送信する
    if(code == 200) ink_println(url);           // 送信URLを表示
    http.end();                                 // HTTP通信を終了する
    sleep();                                    // 下記のsleep関数を実行
}

void sleep(){                                   // スリープ実行用の関数
    delay(100);                                 // 送信完了の待ち時間処理
    WiFi.disconnect();                          // Wi-Fiの切断
    digitalWrite(LED_EXT_PIN, HIGH);            // LED消灯
    ink_println("Elapsed "+String((float)millis()/1000.,1)+" Seconds");
    if(batt_mv() > 3300){                       // 電圧が3300mV以上のとき
        int sec = (int)(SLEEP_P/1000000ul);     // 秒に変換
        ink_println("Sleeping for " + String(sec) + " Seconds");
        M5.shutdown(sec);                       // タイマー・スリープ
    }   // else:
    ink_println("Power OFF");                   // Inkへメッセージを表示
    M5.shutdown();                              // 電源OFF
}

/******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/coreink/arduino

M5Stack Arduino Library API 情報：
https://docs.m5stack.com/en/api/coreink/system_api

BH1750FVI データシート 2011.11 - Rev.D (ローム)

電池電圧の取得：
https://docs.m5stack.com/en/core/coreink
R41 = 20K
R42 = 5.1K
ADC=GPIO35

[BAT]--[R41]--*--[R42]--GND
              |
              +--GPIO35

SY8089 データシート

*******************************************************************************
【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example06_lum
https://github.com/bokunimowakaru/esp/tree/master/2_example/example38_lum
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex03_lum
*******************************************************************************/
