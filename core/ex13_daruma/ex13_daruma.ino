/*******************************************************************************
Example 13 daruma for M5Stack ～PIR Sensor ユニットで だるまさんがころんだ～

                                               Copyright (c) 2022 Wataru KUNINO
*******************************************************************************/

#include <M5Stack.h>                            // M5Stack用ライブラリの組み込み
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiClientSecure.h>                   // TLS(SSL)通信用ライブラリ
#include <HTTPClient.h>                         // HTTP通信用ライブラリ
#include "root_ca.h"                            // HTTPS通信用ルート証明書
#define PIN_PIR 22                              // G22にセンサ(人感/ドア)を接続
#define SSID "1234ABCD"                         // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
#define URL  "https://bokunimo.com/daruma/"     // クラウドサービスのURL
RTC_DATA_ATTR uint32_t pir_delay = 2000;        // 人感センサの解除遅延時間ms(2秒)

/*******************************************************************************
SSIDを送信したくない場合は、下記の USER にニックネームなどを入力してください。
*******************************************************************************/

String USER = "";                               // クラウドへ送信するユーザ名

/*******************************************************************************
ユーザ名 USER は、半角英文字と数字、8文字までを、2つの"の中に入力してください。
例） String USER = "wataru";
********************************************************************************
ユーザ名 USER は下記の目的でアクセス元の端末を特定するために使用します。
なるべく実名を避け、ニックネームなどを使用してください。
（デフォルトでは、SSIDの一部(後部4文字)をユーザ名として利用します）
クラウド・サーバとの通信にはHTTPSを使用し、ユーザ名などの情報は以下の目的、用途で
使用します。

【利用目的・用途】
・送信データの傾向学習用
・システム・アルゴリズムの改良
・ランキング表示や対戦時の表示用
・その他、ネット・サービスの開発用

【ご注意】
・当方の過失による流出については責任を負いません
・データ消失については責任を負いません（誤った削除依頼で消失する場合もあります）
・削除依頼については対応しますが、適正な依頼かどうかは当方の判断に基づきます

同意いただけない場合は、当該クエリ（user=）を削除してください。
*******************************************************************************/

boolean pir;                                    // 人感センサ状態
int bar100 = 0;                                 // 前回の棒グラフ値(100分率)
int barPrev = 0;                                // 前回の棒グラフ値(ピクセル数)
int prisoners = 0;                              // 鬼に捕まったユーザ数

void dispBar(int level=bar100){                 // 棒グラフを描画する関数
    pir = digitalRead(PIN_PIR);                 // 人感センサ値を取得
    bar100 = level;                             // 入力値をbar100に保持
    level = (level * 314) / 100;                // 百分率をピクセル数に変換
    M5.Lcd.drawRect(1, 1, 318, 8, pir*RED);     // LCD上部に棒グラフの枠を表示
    M5.Lcd.drawRect(2, 2, 316, 6, pir*RED);     // 棒グラフの枠の厚みを増やす
    int len = level - barPrev;                  // 棒グラフの増減をlenに代入
    if(0 < len){                                // グラフが短くなったとき
        M5.Lcd.fillRect(3+barPrev,3,len,4,GREEN); // LCD上部に棒グラフを表示
    }else{                                      // グラフが長くなったとき
        M5.Lcd.fillRect(3+level,3,-len,4,WHITE); // LCD上部に棒グラフを表示
    }
    barPrev = level;                            // 現在の値を前回の値として保持
    delay(33);                                  // 33ミリ秒の待ち時間処理
}

void dispText(String msg=""){                   // LCDに文字を表示する
    M5.Lcd.setTextColor(WHITE);                 // テキスト文字の色を設定(白)
    for(int x=-2; x<=2; x+=2) for(int y=-2; y<=2; y+=2) if(x||y){
        M5.Lcd.drawCentreString(msg,160+x,120+y,4);
    }                                           // テキストの背景を描画する
    M5.Lcd.setTextColor(BLACK);                 // テキスト文字の色を設定(黒)
    M5.Lcd.drawCentreString(msg,160,120,4);     // 文字列を表示
    barPrev = 0;                                // 棒グラフが消えるのでリセット
}

void dispPrisoners(int n = prisoners){          // 捕まった人の人数表示
    for(int i=1; i <= 6; i++){                  // 6回の繰り返し処理
        int y = 226 - i * 30;                   // 手錠を表示するY座標を計算
        if(i <= n){
            drawJpgHeadFile("daruma5",0,y);     // 手錠の画像をLCD表示
        }else{
            M5.Lcd.fillRect(0,y,37,30,WHITE);   // 手錠の画像を消去
        }
    }
    M5.Lcd.fillRect(38,68,34,24,WHITE);         // 人数表示(数字)を消去
    if(n>6)M5.Lcd.drawString(String(n),38,68,4); // 6人を超過したときに人数表示
}

int getPrisoners(int n = 0){                    // 鬼に捕まったユーザ数
    String S = String(URL);                     // HTTPリクエスト用の変数
    S += "?user=" + USER;                       // ユーザ名のクエリを追加
    if(n) S += "&n="+ String(n);                // 項目nとその値を追加
    WiFiClientSecure client;                    // TLS/TCP/IP接続部の実体を生成
    client.setCACert(rootCACertificate);        // ルートCA証明書を設定
    // client.setInsecure();                    // サーバ証明書を確認しない
    HTTPClient https;                           // HTTP接続部の実体を生成
    https.begin(client, S);                     // 初期化と接続情報の設定
    int httpCode = https.GET();                 // HTTP接続の開始
    S = https.getString();                      // 受信結果を変数Sへ代入
    // Serial.println(httpCode);                // httpCodeをシリアル出力
    // Serial.println(S);                       // 受信結果をシリアル出力
    if(httpCode == 200){                        // HTTP接続に成功したとき
        n = S.substring(S.indexOf("\"n\":")+5).toInt(); // パースn
    }                                           // パース方法は受信サンプル参照
    https.end();                                // HTTPクライアントの処理を終了
    client.stop();                              // TLS(SSL)通信の停止
    return n;
}

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    pinMode(PIN_PIR,INPUT);                     // センサ接続したポートを入力に
    drawJpgHeadFile("daruma3", 0, 0);           // filenameに応じた画像をLCD表示
    dispText("Example 13 Daruma-san");          // タイトル文字を表示
    drawJpgHeadFile("daruma2", 80, 32);         // 顔を表示
    drawJpgHeadFile("daruma4", 172, 8);         // タイトル画像を表示
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    while(WiFi.status() != WL_CONNECTED);       // 接続に成功するまで待つ
    while(digitalRead(PIN_PIR));                // 非検出状態になるまで待つ
    if(USER == "") USER = String(SSID).substring(String(SSID).length()-4);
    prisoners = getPrisoners();                 // 捕まった人数をサーバから取得
    drawJpgHeadFile("daruma3", 0, 0);           // filenameに応じた画像をLCD表示
    dispText("GAME START");                     // タイトル文字を表示
    delay(1000);                                // 1秒間の待ち時間処理
}

void loop(){                                    // 繰り返し実行する関数
    drawJpgHeadFile("daruma0", 0, 0);           // filenameに応じた画像をLCD表示
    dispPrisoners();                            // 捕まった人数の表示
    M5.Lcd.drawCentreString("BREAK",68,224,2);  // 文字列"BREAK"を表示
    for(int i=0; i <= 100; i++){                // 棒グラフを増加させる
        M5.update();                            // ボタン情報を更新
        dispBar(i);                             // 棒グラフの描画
        if(M5.BtnA.isPressed()){                // Aボタンが押されたとき
            M5.Lcd.fillRect(0,224,128,16,WHITE); // "BREAK"を消去
            drawJpgHeadFile("daruma2", 80, 32); // 顔を表示
            dispText("Cleared!");               // "Cleared"を表示
            prisoners=getPrisoners(-2);         // 2人をサーバから減算
            dispPrisoners();                    // 捕まった人数の表示
            delay(5000);                        // 5秒間の待ち時間処理
            return;                             // loop関数の先頭に戻る
        }
        if(i==33){                              // 棒グラフ33%のとき
            drawJpgHeadFile("daruma4", 172, 8); // タイトル画像を表示
        }
    }
    M5.Lcd.fillRect(0,224,128,16,WHITE);        // "BREAK"を消去
    drawJpgHeadFile("daruma2", 80, 32);         // 顔を表示
    delay(pir_delay);                           // PIRセンサの遅延分の待ち時間
    for(int i=100; i >= 0; i--){                // 棒グラフを減らす処理
        dispBar(i);                             // 棒グラフの描画
        pir = digitalRead(PIN_PIR);             // 人感センサ値を取得
        if(pir){                                // センサ反応時
            dispText("Failed");                 // "Failed"を表示
            prisoners = getPrisoners(1);        // 捕まった人数をサーバに加算
            dispPrisoners();                    // 捕まった人数の表示
            end();                              // 終了関数endを実行
            return;                             // loop関数の先頭に戻る
        }
    }
}

void end(){
    M5.Lcd.drawCentreString("GAME START",256,224,2); // 文字列を表示
    do{                                         // ボタンが押されるまで待機する
        M5.update();                            // ボタン情報を更新
        dispBar();                              // 棒グラフの描画
    }while(!M5.BtnC.wasPressed());              // ボタンが押されるまで繰り返す
}

/*
受信サンプル：
{"statusCode": 200, "body": {"status": "ok", "n": 6}}

簡易パーサ(n) indexOf("\"n\":")+5
        "n": 6
        01234567890123
*/
