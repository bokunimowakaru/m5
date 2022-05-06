/*******************************************************************************
Example 11: ESP32 (IoTセンサ) Wi-Fi BLEビーコン・センサ for M5Stack Core
・Bluetooth LE のアドバタイジング送信数をカウントします。

    使用機材(例)：M5Stack Core

                                          Copyright (c) 2022 Wataru KUNINO
*******************************************************************************/

#include <M5Stack.h>                            // M5Stack用ライブラリの組み込み
#include <BLEDevice.h>                          // BLE通信用ライブラリ
#include <BLEScan.h>                            // BLEビーコンのスキャン用
#include <BLEAdvertisedDevice.h>                // アドバタイズ情報取得用
RTC_DATA_ATTR int disp_max = 8;                 // メータの最大値

BLEScan *pBLEScan;                              // BLEスキャナ用ポインタ

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    M5.Lcd.setBrightness(31);                   // 輝度を下げる（省エネ化）
    analogMeterInit("devices","Counter", 0, disp_max);  // メータの初期表示
    M5.Lcd.println("ex.11 M5Stack BLE Beacon Counter"); // タイトルの表示

    BLEDevice::init("");                        // BLE通信ライブラリの初期化
    pBLEScan = BLEDevice::getScan();            // BLEスキャナの実体化
}

void loop(){                                    // 繰り返し実行する関数
    BLEScanResults devs =(*pBLEScan).start(30); // 30秒間のBLEスキャンの実行
    int count = 0;                              // カウント値を保持する変数count
    for(int i = 0; i < devs.getCount(); i++){   // 発見したBLE機器数の繰り返し
        BLEAdvertisedDevice dev = devs.getDevice(i);    // 発見済BLEの情報を取得
        int rssi = dev.getRSSI();               // RSSI受信強度を取得
        if( rssi >= -80 ) count++;              // -80dBm以上のときにカウント

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

    }
    analogMeterNeedle(count,5);                 // 発見数に応じてメータ針を設定
    (*pBLEScan).clearResults();                 // BLEScanのバッファのクリア

    if(count >= disp_max * 3 / 4){              // メータ値が3/4以上のとき
        M5.Lcd.fillRect(0,178, 320,28,TFT_RED); // 表示部の背景を赤色に塗る
    }else{
        M5.Lcd.fillRect(0,178, 320,28, BLACK);  // 表示部の背景を黒色に塗る
    }
    String S = "BLE Devices = "+String(count);  // count値を文字列変数Sに代入
    M5.Lcd.drawCentreString(S, 160, 180, 4);    // 文字列を表示
    M5.Lcd.setCursor(196, 168);                 // 文字位置を設定
    M5.Lcd.fillRect(196, 168, 124, 8, BLACK);   // 表示部の背景を黒色に塗る
}

/*******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/m5core/arduino

M5Stack Arduino Library API 情報：
https://docs.m5stack.com/en/api/core/system

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
