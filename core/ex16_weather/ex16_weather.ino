/*******************************************************************************
Example 16: 天気予報を表示する IoT TeleTele坊主 for M5Stack

                                          Copyright (c) 2019-2023 Wataru KUNINO
********************************************************************************

# 【重要なご注意】
# 本ソフトウェアの利用に関して、筆者(国野 亘)は、責任を負いません。
# 気象業務法や、下記の予報業務許可に関する情報、上記参考文献の注意事項を
# 良く読んでから利用ください。
# 気象業務法   https://www.jma.go.jp/jma/kishou/info/ml-17.html
# 予報業務許可 https://www.jma.go.jp/jma/kishou/minkan/q_a_m.html

地域設定：
    ・#define CITY_ID に地域コードを設定してください。
            # 気象庁=130000(東京地方など)
            # 大阪管区気象台=270000(大阪府など)
            # 京都地方気象台=260000(南部など)
            # 横浜地方気象台=140000(東部など)
            # 銚子地方気象台=120000(北西部など)
            # 名古屋地方気象台=230000(西部など)
            # 福岡管区気象台=400000(福岡地方など)

https://github.com/bokunimowakaru/m5s
https://github.com/bokunimowakaru/m5s/blob/master/example03_wea/example03_wea.ino

# 【重要なご注意】
# livedoor 天気 のサービス終了に伴い、上記の互換サービスを利用します。
# 同サービスは、気象庁による気象予報をlivedoor天気の互換形式に変換して配信します。
# (2023年1月28日現在、気象予報そのものは行っていない)
# 同サービスや本ソフトウェアの利用に関して、筆者(国野 亘)は、責任を負いません。
# 気象業務法や、下記の予報業務許可に関する情報、上記参考文献の注意事項を
# 良く読んでから利用ください。
# 気象業務法   https://www.jma.go.jp/jma/kishou/info/ml-17.html
# 予報業務許可 https://www.jma.go.jp/jma/kishou/minkan/q_a_m.html
*******************************************************************************/

#include <M5Stack.h>                            // M5Stack用ライブラリの組み込み
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#define SSID "1234ABCD"                         // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
#define CITY_ID 270000                             // 県番号:東京13,福島7,愛知23
                                                // 大阪27,京都26,兵庫28,熊本43
const char wtrFiles[5][13]={ "wtr_uknw_jpg", "wtr_rain_jpg", "wtr_clud_jpg",
                             "wtr_fine_jpg", "wtr_snow_jpg" };

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    drawJpgHeadFile(wtrFiles[0]);        // LCDを消去。背景画像表示
    M5.Lcd.setTextColor(BLACK);                 // 文字色を黒色に
    M5.Lcd.setTextSize(2);                      // 文字表示サイズを2倍に設定
}

void loop(){                                    // 繰り返し実行する関数
    WiFi.mode(WIFI_STA);                        // 無線LANを【子機】モードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    M5.Lcd.setCursor(0,0);                      // カーソル位置を画面左上に
    M5.Lcd.println("Example 16 Weather");       // LCDにタイトルを表示
    while(WiFi.status() != WL_CONNECTED){       // 接続に成功するまで待つ
        delay(500);                             // 待ち時間処理(LED点滅用)
        M5.Lcd.print('.');                      // 接続用プログレス表示
    }
    char s[17];                                 // 文字列変数を定義
    int weather = httpGetWeather(CITY_ID,s,17); // 天気情報を取得
    WiFi.disconnect(true);                      // WiFiアクセスポイントを切断
    WiFi.mode(WIFI_OFF);                        // 無線LANをOFFに設定する
    drawJpgHeadFile(wtrFiles[weather]);  // 天気の画像を表示
    M5.Lcd.drawCentreString(s,164,200,2);       // 天気予報情報表示
    while(1){
        if(millis() % 3600000ul < 500) return;  // 1時間に1回だけ天気を取得
        M5.update();                            // ボタン情報を更新
        if( M5.BtnA.wasPressed() ){             // ボタンAが押されていた時
            drawJpgHeadFile(wtrFiles[0]); // 背景画像表示
            return;                             // 再取得を実行
        }
        if( M5.BtnB.read() && M5.BtnC.wasPressed() ){   // ボタンBを押しながらC
            weather++;                                  // 天気を変更
            if(weather > 4) weather = 1;                // 天気が4を超えたら1に
            drawJpgHeadFile(wtrFiles[weather]);  // 天気の画像を表示
            M5.Lcd.drawCentreString(s,164,200,2);       // 天気予報情報表示
        }
    }
}
