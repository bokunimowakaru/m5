/*******************************************************************************
Example 5: ESP32 (IoTセンサ) Wi-Fi 温湿度計 SENSIRION製 SHT30/SHT31/SHT35 版
                                                           for M5Stack CORE INK
【折れ線グラフ対応】

・デジタルI2Cインタフェース搭載センサから取得した温湿度を送信するIoTセンサです。
・電子ペーパー搭載 M5Stack Core Ink に折れ線グラフで温度と湿度を表示します。

    使用機材(例)：M5Stack CORE INK + ENV II/III HAT

    M5Stack CORE INK のI2Cポート:
        SHT30/SHT31/SHT35 SDAポート G25
        SHT30/SHT31/SHT35 SCLポート G26
        HAT 設定方法＝shtSetup(25,26);

    ENV II/III HAT の代わりに ENV II/III UNIT (Grove互換端子)を使用するときは
    ポート番号の変更が必要です。
        UNIT設定方法＝shtSetup(32,33);

注意: ENV HATのバージョンによって搭載されているセンサが異なります。
      このプログラムは SHT30 用です。初代 ENV HAT には対応していません。

ENV HAT     DHT12 + BMP280 + BMM150
ENV II HAT  SHT30 + BMP280 + BMM150
ENV III HAT SHT30 + QMP6988

                                          Copyright (c) 2016-2023 Wataru KUNINO
*******************************************************************************/

#include <M5CoreInk.h>                          // M5Stack用ライブラリ組み込み
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ
#include "esp_sleep.h"                          // ESP32用Deep Sleep ライブラリ
#include "driver/rtc_io.h"                      // ESP32側のRTC IO用ライブラリ

#define SSID "1234ABCD"                         // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
#define PORT 1024                               // 送信のポート番号
#define SLEEP_P 60*1000000ul                    // スリープ時間 60秒(uint32_t)
#define DEVICE "humid_5,"                       // デバイス名(5字+"_"+番号+",")

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

RTC_DATA_ATTR uint8_t PageBuf[200*200/8];       // ESP32用メモリの確保(画像用)
RTC_DATA_ATTR uint32_t ink_refresh_time;        // e-paper全消去後の経過時間
Ink_Sprite InkPageSprite(&M5.M5Ink);            // e-paper描画用インスタンス
int wake = (int)esp_sleep_get_wakeup_cause();   // 起動理由を変数wakeに保存

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
    /* CoreInk側のRTC起動時に変数wakeを上書きする処理(ESP32側RTC使用時は不要)
    if(!wake){                                  // ESP32が電源による起動の場合
        Wire1.begin(21, 22);                    // RTCによる起動を確認する処理
        uint8_t data = M5.rtc.ReadReg(0x01);    // (M5.beginより前に配置する)
        if(data & 0x04) wake = ESP_SLEEP_WAKEUP_TIMER; // タイマー起動に上書き
    } (ここまで)CoreInk側RTC起動時の処理 */
    
    M5.begin();                                 // M5Stack用ライブラリの起動
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    shtSetup(25,26);                            // 湿度センサの初期化
    if(wake != ESP_SLEEP_WAKEUP_TIMER){         // タイマー以外で起動時の処理
        while(!M5.M5Ink.isInit()) delay(3000);  // Inkの初期化
        M5.M5Ink.clear();                       // Inkを消去
        ink_refresh_time = 0;                   // 消去した時刻を0に
        InkPageSprite.creatSprite(0,0,200,200,0);   // 描画用バッファの作成
        lineGraphInit(&InkPageSprite, 16, 0, 100);  // グラフ初期化,縦軸範囲指定
        ink_print_init(&InkPageSprite);             // テキスト表示用 ink_print
        ink_print("Example 5 HUM",false);           // タイトルの描画
    }else{                                          // タイマー起動時の処理
        while(!M5.M5Ink.isInit()) delay(3000);      // Inkの初期化
        InkPageSprite.creatSprite(0,0,200,200,0);   // 描画用バッファの作成
        InkPageSprite.drawFullBuff(PageBuf);        // RTCメモリから画像読み込み
        lineGraphSetSprite(&InkPageSprite, 16, 0, 100); // 棒グラフ描画用の設定
        ink_print_setup(&InkPageSprite);            // テキスト表示用 ink_print
        if(ink_refresh_time >= 60*60*1000){         // 1時間が経過
            ink_refresh_time = 0;                   // 消去した時刻を0に
            InkPageSprite.pushSprite();             // e-paperに描画
            delay(300);
            InkPageSprite.clear();                  // 画面の消去
            InkPageSprite.pushSprite();             // e-paperに描画
            InkPageSprite.deleteSprite();           // 描画用バッファの削除
            InkPageSprite.creatSprite(0,0,200,200,0); // 描画用バッファの作成
            while(!M5.M5Ink.isInit()) delay(300);   // Inkの初期化
            M5.M5Ink.clear();                       // Inkを消去
            lineGraphCls();                         // グラフ画面の罫線描画
            lineGraphRedraw();                      // 過去グラフの再描画
            ink_print("Example 5 HUM",false);       // タイトルの描画
        }
    }
    InkPageSprite.pushSprite();                 // e-paperに描画

    ink_printPos(120,0);                        // 文字表示位置を移動
    ink_print("("+String(wake)+")",false);      // 起動値をバッファに描画
    ink_printPos(144,0);                        // 文字表示位置を移動
    ink_print(String(batt_mv())+" mV",false);   // 電圧値をバッファに描画
    InkPageSprite.FillRect(0,160,200,40,1);     // テキスト文字エリアを消去
    ink_printPos(160);
    /*
    char s[8] = "*** mV";                       // 文字列変数sを生成
    snprintf(s,4,"(%d)",wake);                  // 起動値を文字列変数sに代入
    InkPageSprite.drawString(120,0,s);          // 起動値をバッファに描画
    snprintf(s,9,"%d mV",batt_mv());            // 電圧値を文字列変数sに代入
    InkPageSprite.drawString(144,0,s);          // 電圧値をバッファに描画
    */
}

void loop(){                                    // 繰り返し実行する関数
    float temp = getTemp();                     // 温度を取得して変数tempに代入
    float hum = getHum();                       // 湿度を取得して変数humに代入
    int batt = batt_mv();                       // 電池電圧を取得してbattに代入
    
    drawNum(&InkPageSprite, 128, String(temp,1)+"C "+String(hum,0)+"%");
    if(temp < -100. || hum < 0.) sleep();       // 取得失敗時に末尾のsleepを実行

    String S = String(DEVICE);                  // 送信データSにデバイス名を代入
    S += String(temp,1) + ", ";                 // 変数tempの値を追記
    S += String(hum,1) + ", ";                  // 変数humの値を追記
    S += String(batt);                          // 変数battの値を追記
    Serial.println(S);  // debug

    while(WiFi.status() != WL_CONNECTED){       // 接続に成功するまで待つ
        if(millis() > 30000) sleep();           // 30秒超過でスリープ
        if(millis()%1000 == 0){                 // 1秒間の描画待機用の処理
            InkPageSprite.pushSprite();         // e-paperに描画
            return;                             // loopの先頭に戻る(更新用)
        }
    }
    
    lineGraphPlot(temp,0);                      // tempをグラフ表示
    lineGraphPlot(hum,1);                       // humをグラフ表示
    lineGraphPlot((float)batt*100/4300,2);      // battをグラフ表示

    WiFiUDP udp;                                // UDP通信用のインスタンスを定義
    udp.beginPacket(UDPTO_IP, PORT);            // UDP送信先を設定
    udp.println(S);                             // 送信データSをUDP送信
    udp.endPacket();                            // UDP送信の終了(実際に送信する)
    if(strcmp(Amb_Id,"00000") == 0) sleep();    // Ambient未設定時にsleepを実行

    S = "{\"writeKey\":\""+String(Amb_Key);     // (項目)writeKey,(値)ライトキー
    S += "\",\"d1\":\"" + String(temp,2);       // (項目)d1,(値)温度
    S += "\",\"d2\":\"" + String(hum,2);        // (項目)d2,(値)湿度
    S += "\",\"d3\":\"" + String(batt) + "\"}"; // (項目)d3,(値)電圧

    HTTPClient http;                            // HTTPリクエスト用インスタンス
    http.setConnectTimeout(15000);              // タイムアウトを15秒に設定する
    String url = "http://ambidata.io/api/v2/channels/"+String(Amb_Id)+"/data";
    http.begin(url);                            // HTTPリクエスト先を設定する
    http.addHeader("Content-Type","application/json"); // JSON形式を設定する
    int code = http.POST(S);                    // センサ値をAmbientへ送信する
    if(code == 200) ink_print(url.substring(0,22)+"...",false); // 送信URL表示
    http.end();                                 // HTTP通信を終了する
    sleep();                                    // 下記のsleep関数を実行
}

void sleep(){                                   // スリープ実行用の関数
    M5.update();                                // M5Stack用IO状態の更新
    InkPageSprite.pushSprite();                 // e-paperに描画
    // delay(100);                              // 送信完了の待ち時間処理
    WiFi.disconnect();                          // Wi-Fiの切断
    memcpy(PageBuf,InkPageSprite.getSpritePtr(),200*200/8); // ESP内のRTCに保存
    ink_refresh_time += millis() + SLEEP_P/1000;
    Serial.println(ink_refresh_time/1000);  // debug
    
    digitalWrite(LED_EXT_PIN, HIGH);            // LED消灯
    // ink_println("Elapsed "+String((float)millis()/1000.,1)+" Seconds");
    if(batt_mv() > 3300 && !M5.BtnPWR.wasPressed()){ // 電圧が3300mV以上のとき
        /* スリープ中に GPIO12 をHighレベルに維持する(ESP32への電源供給) */
        rtc_gpio_init(GPIO_NUM_12);
        rtc_gpio_set_direction(GPIO_NUM_12,RTC_GPIO_MODE_OUTPUT_ONLY);
        rtc_gpio_set_level(GPIO_NUM_12,1);
        esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
        unsigned long us = millis() * 1000ul + 363000ul;
        if(SLEEP_P > us) us = SLEEP_P - us; else us = 1000000ul;
        ink_println("Sleeping for " + String(((double)(us/100000))/10.,1) + " secs");
        esp_deep_sleep(us);                     // Deep Sleepモードへ移行
        
        /* 下記の方法では,GPIO12を保持できない(2023年6月時点)
        int sec = (int)(SLEEP_P/1000000ul);     // 秒に変換
        sec -= (millis()-500)/1000;             // 動作時間を減算
        if(sec < 1) sec = 1;
        ink_println("Sleeping for " + String(sec) + " Seconds");
        M5.shutdown(sec);                       // タイマー・スリープ
        */
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

電池電圧の取得：
https://docs.m5stack.com/en/core/coreink
R41 = 20K
R42 = 5.1K
ADC=GPIO35

[BAT]--[R41]--*--[R42]--GND
              |
              +--GPIO35

SY8089 データシート

起動理由の取得：
https://github.com/m5stack/M5Core-Ink/blob/master/examples/Basics/RTC_Clock/RTC_Clock.ino
    // Check power on reason before calling M5.begin()
    //  which calls Rtc.begin() which clears the timer flag.
    Wire1.begin(21, 22);
    uint8_t data = M5.rtc.ReadReg(0x01);

M5 Core Ink 回路図：
https://docs.m5stack.com/en/core/coreink
PWR_SW&ADC：GPIO12で5V DCDCと3.3V DC-DCの動作を維持できる

*******************************************************************************
【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example09_hum_sht31
https://github.com/bokunimowakaru/esp/tree/master/2_example/example41_hum_sht31
https://github.com/bokunimowakaru/m5s/tree/master/example04d_temp_hum_sht
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex05_hum
*******************************************************************************/
