/*******************************************************************************
Example 5: ESP32 (IoTセンサ) Wi-Fi 温湿度計 SENSIRION製 SHT30/SHT31/SHT35 版
・デジタルI2Cインタフェース搭載センサから取得した温湿度を送信するIoTセンサです。

    使用機材(例)：M5Sack Core + ENV II/III Unit

注意: ENV HATのバージョンによって搭載されているセンサが異なります。
      このプログラムは SHT30 用です。ENV Unit には対応していません。

ENV Unit     DHT12 + BMP280 + BMM150 ※非対応
ENV II Unit  SHT30 + BMP280 + BMM150
ENV III Unit SHT30 + QMP6988

                                          Copyright (c) 2016-2022 Wataru KUNINO
*******************************************************************************/

#include <M5Stack.h>                            // M5Stack用ライブラリの組み込み
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ

#define SSID "1234ABCD"                         // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
#define PORT 1024                               // 送信のポート番号
#define SLEEP_P 30*1000000ul                    // スリープ時間 30秒(uint32_t)
#define DEVICE "humid_3,"                       // デバイス名(5字+"_"+番号+",")
RTC_DATA_ATTR int disp_min = 14;                // 折れ線グラフの最小値
RTC_DATA_ATTR int disp_max = 34;                // 折れ線グラフの最大値

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
    M5.begin();                                 // M5Stack用ライブラリの起動
    shtSetup();                                 // 湿度センサの初期化
    M5.Lcd.setBrightness(31);                   // 輝度を下げる（省エネ化）
    analogMeterInit("Celsius",0,40,"RH%",0,100); //メータ初期化
    analogMeterSetNames("Temp.","Humi.");       // メータのタイトルを登録
    lineGraphInit(disp_min,disp_max);           // グラフ初期化(縦軸の範囲指定)
    M5.Lcd.println("ex.05 M5Stack Temp & Hum (SHT30)");
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
}

void loop(){                                    // 繰り返し実行する関数
    M5.update();                                // ボタン状態の取得
    delay(1);                                   // ボタンの誤作動防止
    int btn=M5.BtnA.wasPressed()+2*M5.BtnB.wasPressed()+4*M5.BtnC.wasPressed();
    switch(btn){
        case 1: disp_min = 24; disp_max = 34; break;  // 夏季向けの表示
        case 2: disp_min = 14; disp_max = 34; break;  // 通常の表示
        case 4: disp_min = 14; disp_max = 24; break;  // 冬季向けの表示
        default: btn = 0; break;
    }
    if(btn) lineGraphInit(disp_min,disp_max);   // ボタン操作時にグラフ初期化
    if(millis()%(SLEEP_P/1000) == 0){           // SLEEP_P間隔で下記を実行
        WiFi.begin(SSID,PASS);                  // 無線LANアクセスポイントへ接続
    }
    if(millis()%500) return;                    // 以下は500msに1回だけ実行する

    M5.Lcd.fillRect(283, 194, 37, 8, BLACK);    // Wi-Fi接続の待ち時間
    M5.Lcd.setCursor(283, 194);                 // 文字位置を設定
    M5.Lcd.printf("(%d) ",WiFi.status());        // Wi-Fi状態番号を表示
    M5.Lcd.print((SLEEP_P/1000 - millis()%(SLEEP_P/1000))/1000);

    float temp = getTemp();                     // 温度を取得して変数tempに代入
    float hum = getHum();                       // 湿度を取得して変数humに代入
    if(temp < -100. || hum < 0.) return;        // 取得失敗時に戻る

    float wbgt = 0.725*temp + 0.0368*hum + 0.00364*temp*hum - 3.246 + 0.5;
    analogMeterNeedle(0,temp);                  // メータに温度を表示
    analogMeterNeedle(1,hum);                   // メータに湿度を表示
    lineGraphPlot(wbgt);                        // WBGTをグラフ表示
    if(12. < wbgt && wbgt < 30.){               // 12℃より大かつ30℃より小の時
        M5.Lcd.fillRect(0,210, 320,30, BLACK);  // 表示部の背景を塗る
    }else{
        M5.Lcd.fillRect(0,210, 320,30,TFT_RED); // 表示部の背景を塗る
    }

    String S = "WBGT= " + String(wbgt,1);       // WBGT値を文字列変数Sに代入
    S += "C ("+String(temp,1)+"C, "+String(hum,0)+"%)"; // 温度と湿度をSに追記
    M5.Lcd.drawCentreString(S, 160, 210, 4);    // 文字列を表示
    if(WiFi.status() != WL_CONNECTED) return;   // Wi-Fi未接続のときに戻る

    M5.Lcd.fillRect(0, 202, 320, 8, BLACK);     // 表示部の背景を塗る
    M5.Lcd.setCursor(0, 202);                   // 文字位置を設定
    M5.Lcd.println(WiFi.localIP());             // 本機のアドレスをLCDに表示

    S = String(DEVICE);                         // 送信データSにデバイス名を代入
    S += String(temp,1) + ", ";                 // 変数tempの値を追記
    S += String(hum,1);                         // 変数humの値を追記
    Serial.println(S);                          // 送信データSをシリアル出力表示
    WiFiUDP udp;                                // UDP通信用のインスタンスを定義
    udp.beginPacket(UDPTO_IP, PORT);            // UDP送信先を設定
    udp.println(S);                             // 送信データSをUDP送信
    udp.endPacket();                            // UDP送信の終了(実際に送信する)

    if(strcmp(Amb_Id,"00000") != 0){            // Ambient設定時に以下を実行
        S = "{\"writeKey\":\""+String(Amb_Key); // (項目)writeKey,(値)ライトキー
        S += "\",\"d1\":\"" + String(temp,2);   // (項目)d1,(値)温度
        S += "\",\"d2\":\"" + String(hum,2);    // (項目名)d2,(値)湿度
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

/******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/m5core/arduino

M5Stack Arduino Library API 情報：
https://docs.m5stack.com/en/api/core/system

【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example09_hum_sht31
https://github.com/bokunimowakaru/esp/tree/master/2_example/example41_hum_sht31
https://github.com/bokunimowakaru/m5s/tree/master/example04d_temp_hum_sht
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex05_hum
*******************************************************************************/
