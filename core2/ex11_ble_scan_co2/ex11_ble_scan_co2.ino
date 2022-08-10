/*******************************************************************************
Example 11: ESP32 (IoTセンサ) Wi-Fi BLEビーコン+CO"・センサ for M5Stack Core2
・Bluetooth LE のアドバタイジング送信数を送信するIoTセンサです。
・TVOC/eCO2 UNIT の測定結果も合わせて表示することで室内の換気状況も推測できます。

    使用機材(例)：M5Stack Core2 + TVOC/eCO2 UNIT

※M5Stackのボタン状態は30秒に1回しか確認しないので、ボタンを押しながらリセット操作をするか
　レンジが変わるまで待ち続けてください。

                                          Copyright (c) 2022 Wataru KUNINO
*******************************************************************************/

#include <M5Core2.h>                            // M5Stack用ライブラリの組み込み
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ
#include <BLEDevice.h>                          // BLE通信用ライブラリ
#include <BLEScan.h>                            // BLEビーコンのスキャン用
#include <BLEAdvertisedDevice.h>                // アドバタイズ情報取得用
#define SSID "1234ABCD"                         // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
#define PORT 1024                               // 送信のポート番号
#define SLEEP_P 30*1000000ul                    // Wi-Fi送信間隔 30秒(uint32_t)
#define DEVICE "count_3,"                       // デバイス名(5字+"_"+番号+",")
RTC_DATA_ATTR int disp_max = 8;                 // メータの最大値
int line_max = 10;

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

/******************************************************************************
 UDP 宛先 IP アドレス設定
 ******************************************************************************
 カンマ区切りでUPD宛先IPアドレスを設定してください。
 末尾を255にすると接続ネットワーク(アクセスポイント)にブロードキャスト
 *****************************************************************************/
IPAddress UDPTO_IP = {255,255,255,255};         // UDP宛先 IPアドレス

BLEScan *pBLEScan;                              // BLEスキャナ用ポインタ

void print_co2(int co2, int tvoc){
    M5.Lcd.setCursor(0,230);
    M5.Lcd.print("CO2 = "+String(co2)+"(ppm), TVOC = "+String(tvoc)+"(ppb)");
}

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    M5.Lcd.setBrightness(31);                   // 輝度を下げる（省エネ化）
    analogMeterSetName(0,"Counter");
    analogMeterSetName(1,"CO2");
    analogMeterInit("devices", 0, disp_max, "ppm", 0, 1000);
    lineGraphInit(0, line_max);                 // グラフ初期化(縦軸の範囲指定)
    M5.Lcd.setCursor(0,194);
    M5.Lcd.println("ex.11 M5Stack BLE Beacon Counter + CO2 "); // タイトルの表示

    sgp30_Setup();                              // CO2センサSGP30を初期化
    int co2 = sgp30_getCo2();
    print_co2(co2,sgp30_getTvoc());             // CO2センサ値を表示
    analogMeterNeedle(1,co2);                   // メータ針を移動

    BLEDevice::init("");                        // BLE通信ライブラリの初期化
    pBLEScan = BLEDevice::getScan();            // BLEスキャナの実体化
    // analogMeterNeedle(0,(*pBLEScan).start(5).getCount()); // メータ針を移動
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイント接続
}

void loop(){                                    // 繰り返し実行する関数
    M5.update();                                // ボタン状態の取得
    int btn=M5.BtnA.isPressed()+2*M5.BtnB.isPressed()+4*M5.BtnC.isPressed();
    switch(btn){
        case 1: disp_max = 8; line_max = 10; break;            // メータ最大値8 家庭向け
        case 2: disp_max = 32; line_max = 30; break;           // メータ最大値32 会議室向け
        case 4: disp_max = 120; line_max = 100; break;          // メータ最大値120 大人数用
        default: btn = 0; break;                // 押されていない時、誤作動時
    }
    if(btn) analogMeterInit("devices", 0, disp_max, "ppm", 0, 1000); // グラフ初期化

    BLEScanResults devs =(*pBLEScan).start(30); // 30秒間のBLEスキャンの実行
    int count = 0;                              // カウント値を保持する変数count
    for(int i = 0; i < devs.getCount(); i++){   // 発見したBLE機器数の繰り返し
        BLEAdvertisedDevice dev = devs.getDevice(i);    // 発見済BLEの情報を取得
        int rssi = dev.getRSSI();               // RSSI受信強度を取得
        if( rssi >= -80 ) count++;              // -80dBm以上のときにカウント
        /*
        // BLEアドレスの取得とシリアル出力
        BLEAddress mac = dev.getAddress();
        Serial.printf("%d, %s, %d, ", i+1, mac.toString().c_str(), rssi);

        // BLEデバイス名の取得とシリアル出力
        String name = dev.getName().c_str();
        Serial.print(name + ", ");

        // ペイロードの取得とシリアル出力
        uint8_t *data = dev.getPayload();
        int data_n = dev.getPayloadLength();
        Serial.print(String(data_n) + ", ");
        for(int i=0 ; i<data_n; i++) Serial.printf("%02X ", data[i]);
        Serial.println();
        */
    }
    analogMeterNeedle(0,count,5);               // 発見数に応じてメータ針を設定
    lineGraphPlot(count,0);
    (*pBLEScan).clearResults();                 // BLEScanのバッファのクリア

    if(count >= disp_max * 3 / 4){              // メータ値が3/4以上のとき
        M5.Lcd.fillRect(0,204, 320,36,TFT_RED); // 表示部の背景を赤色に塗る
    }else{
        M5.Lcd.fillRect(0,204, 320,36, BLACK);  // 表示部の背景を黒色に塗る
    }
    String S = "BLE Devices = "+String(count);  // count値を文字列変数Sに代入
    M5.Lcd.drawCentreString(S, 160, 206, 4);    // 文字列を表示

    int co2 = sgp30_getCo2();                   // CO2センサ値を取得
    print_co2(co2,sgp30_getTvoc());             // CO2センサ値を表示
    analogMeterNeedle(1,co2);                   // メータ針を移動
    lineGraphPlot(co2 * line_max / 1000, 1);

    M5.Lcd.setCursor(232, 194);                 // 文字位置を設定
    M5.Lcd.fillRect(232, 194, 88, 8, BLACK);    // 表示部の背景を黒色に塗る
    if(WiFi.status() != WL_CONNECTED){          // Wi-Fiが未接続の時
        M5.Lcd.print("WiFi ERROR");             // エラーをLCDに表示
        WiFi.disconnect();                      // Wi-Fiの切断
        delay(5000);                            // 待ち時間処理
        WiFi.begin(SSID,PASS);                  // 無線LANアクセスポイント接続
        return;                                 // Wi-Fi未接続のときに戻る
    }
    M5.Lcd.print(WiFi.localIP());               // 本機のアドレスをLCDに表示

    S = String(DEVICE) + String(count);         // 送信データSにデバイス名を代入
    S += ", " + String(co2);
    Serial.println(S);                          // 送信データSをシリアル出力表示
    WiFiUDP udp;                                // UDP通信用のインスタンスを定義
    udp.beginPacket(UDPTO_IP, PORT);            // UDP送信先を設定
    udp.println(S);                             // 送信データSをUDP送信
    udp.endPacket();                            // UDP送信の終了(実際に送信する)

    HTTPClient http;                            // HTTPリクエスト用インスタンス
    http.setConnectTimeout(15000);              // タイムアウトを15秒に設定する
    String url;                                 // URLを格納する変数を生成
    if(strlen(LINE_TOKEN) > 42 && count >= disp_max * 3 / 4){ // LINE送信条件
        url = "https://notify-api.line.me/api/notify";  // LINEのURLを代入
        http.begin(url);                        // HTTPリクエスト先を設定する
        http.addHeader("Content-Type","application/x-www-form-urlencoded");
        http.addHeader("Authorization","Bearer " + String(LINE_TOKEN));
        http.POST("message=密集度は "+String(count)+"(CO2="+String(co2)+"ppm) です。");
        http.end();                             // HTTP通信を終了する
    }
    if(strcmp(Amb_Id,"00000") != 0){            // Ambient設定時に以下を実行
        S = "{\"writeKey\":\""+String(Amb_Key); // (項目)writeKey,(値)ライトキー
        S += "\",\"d1\":\"" + String(count);    // (項目)d1,(値)count
        S += "\",\"d2\":\"" + String(co2);    // (項目)d2,(値)co2
        S += "\"}";
        url = "http://ambidata.io/api/v2/channels/"+String(Amb_Id)+"/data";
        http.begin(url);                        // HTTPリクエスト先を設定する
        http.addHeader("Content-Type","application/json"); // JSON形式を設定する
        http.POST(S);                           // センサ値をAmbientへ送信する
        http.end();                             // HTTP通信を終了する
    }
}

/*******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/core2/arduino

M5Stack Arduino Library API 情報：
https://docs.m5stack.com/en/api/core2/system

ESP32 BLE for Arduino：
https://github.com/espressif/arduino-esp32/tree/master/libraries/BLE
https://github.com/espressif/arduino-esp32/blob/master/libraries/BLE/examples/BLE_scan/BLE_scan.ino
   ************************************************************************************************
   Based on Neil Kolban example for IDF:
   https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
   ************************************************************************************************
https://github.com/espressif/arduino-esp32/blob/master/libraries/BLE/src/BLEScan.h
https://github.com/espressif/arduino-esp32/blob/master/libraries/BLE/src/BLEDevice.h
https://github.com/espressif/arduino-esp32/blob/master/libraries/BLE/src/BLEAddress.h
*******************************************************************************/
