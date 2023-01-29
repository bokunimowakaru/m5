/*******************************************************************************
Example 03: 天気予報を表示する IoT てるてる坊主 for M5Stack

・Yahoo!天気・災害の情報を商用で利用する場合はYahoo! Japanの承諾が必要です。
・天気予報情報の取得結果に応じたJPEG画像を表示します。
・予め以下のファイルをMicro SDカードに保存しておいてください。
    wtr_fine.jpg
    wtr_clud.jpg
    wtr_rain.jpg
    wtr_snow.jpg
    wtr_uknw.jpg

                                          Copyright (c) 2019-2020 Wataru KUNINO
*******************************************************************************/

#include <M5Stack.h>                            // M5Stack用ライブラリの組み込み
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#define SSID "1234ABCD"                         // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
#define PREF 27                                 // 県番号:東京13,福島7,愛知23
                                                // 大阪27,京都26,兵庫28,熊本43
const char wtrFiles[5][14]={ "/wtr_uknw.jpg", "/wtr_rain.jpg", "/wtr_clud.jpg",
                             "/wtr_fine.jpg", "/wtr_snow.jpg" };

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    M5.Lcd.drawJpgFile(SD, wtrFiles[0]);        // LCDを消去。背景画像表示
    M5.Lcd.setTextColor(BLACK);                 // 文字色を黒色に
    M5.Lcd.setTextSize(2);                      // 文字表示サイズを2倍に設定
}

void loop(){                                    // 繰り返し実行する関数
    WiFi.mode(WIFI_STA);                        // 無線LANを【子機】モードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    M5.Lcd.setCursor(0,0);                      // カーソル位置を画面左上に
    M5.Lcd.println("Example 03 Weather");       // LCDにタイトルを表示
    while(WiFi.status() != WL_CONNECTED){       // 接続に成功するまで待つ
        delay(500);                             // 待ち時間処理(LED点滅用)
        M5.Lcd.print('.');                      // 接続用プログレス表示
    }
    char s[17];                                 // 文字列変数を定義
    int weather = httpGetWeather(PREF,s,16);    // 天気情報を取得
    WiFi.disconnect(true);                      // WiFiアクセスポイントを切断
    WiFi.mode(WIFI_OFF);                        // 無線LANをOFFに設定する
    M5.Lcd.drawJpgFile(SD, wtrFiles[weather]);  // 天気の画像を表示
    M5.Lcd.drawCentreString(s,164,200,2);       // 天気予報情報表示
    while(1){
        if(millis() % 3600000ul < 500) return;  // 1時間に1回だけ天気を取得
        M5.update();                            // ボタン情報を更新
        if( M5.BtnA.wasPressed() ){             // ボタンAが押されていた時
            M5.Lcd.drawJpgFile(SD,wtrFiles[0]); // 背景画像表示
            return;                             // 再取得を実行
        }
        if( M5.BtnB.read() && M5.BtnC.wasPressed() ){   // ボタンBを押しながらC
            weather++;                                  // 天気を変更
            if(weather > 4) weather = 1;                // 天気が4を超えたら1に
            M5.Lcd.drawJpgFile(SD,wtrFiles[weather]);   // 天気の画像を表示
            M5.Lcd.drawCentreString(s,164,200,2);       // 天気予報情報表示
        }
    }
}
