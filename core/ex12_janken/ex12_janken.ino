/*******************************************************************************
Example 12 janken for M5Stack ～クラウド・サーバとジャンケン対決～

・クラウド・サーバ bokunimo.com に、Webインタフェース HTTP GET 接続

引用元：
https://github.com/bokunimowakaru/m5Janken
https://github.com/bokunimowakaru/m5Janken/tree/master/janken05_net

説明書：
https://github.com/bokunimowakaru/m5Janken/blob/master/README.md

                                          Copyright (c) 2020-2022 Wataru KUNINO
*******************************************************************************/

#include <M5Stack.h>                            // M5Stack用ライブラリの組み込み
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiClientSecure.h>                   // TLS(SSL)通信用ライブラリ
#include <HTTPClient.h>                         // HTTP通信用ライブラリ
#include "root_ca.h"                            // HTTPS通信用ルート証明書

#define SSID "1234ABCD"                         // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
#define URL  "https://bokunimo.com/janken/"     // クラウドサービスのURL

/*******************************************************************************
SSIDを送信したくない場合は、下記の USER にニックネームなどを入力してください。
*******************************************************************************/

String USER = "";                               // クラウドへ送信するユーザ名

/*******************************************************************************
ユーザ名 USER は、半角英文字と数字、8文字までを、"と"の中に入力してください。
例） String USER = "wataru";
********************************************************************************
ユーザ名 USER は下記の目的でアクセス元の端末を特定するために使用します。
なるべく実名を避け、ニックネームなどを使用してください。
（デフォルトでは、SSIDの一部(後部4文字)をユーザ名として利用します）
クラウド・サーバとの通信にはHTTPSを使用し、ユーザ名などの情報は以下の目的、用途で
使用します。

【利用目的・用途】
・ジャンケンの手の学習用
・ジャンケン・アルゴリズムの改良
・ランキング表示や対戦時の表示用
・その他、ネット・サービスの開発用

【ご注意】
・当方の過失による流出については責任を負いません
・データ消失については責任を負いません（誤った削除依頼で消失する場合もあります）
・削除依頼については対応しますが、適正な依頼かどうかは当方の判断に基づきます

同意いただけない場合は、当該クエリ（user=）を削除してください。
*******************************************************************************/

int rate  = 0;                                  // 勝率

void disp(String filename, String msg=""){      // LCDにJPEGファイルを表示する
    drawJpgHeadFile(filename);                  // filenameに応じた画像をLCD表示
    M5.Lcd.drawCentreString(msg, 160, 96, 4);   // 中央にメッセージ文字列を表示
}

void setup() {
    M5.begin();                                 // M5Stack用ライブラリの起動
    M5.Lcd.setTextColor(TFT_BLACK);             // テキスト文字を黒に設定
    disp("janken","Connecting to Wi-Fi");       // 背景＋接続中表示
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    while(WiFi.status() != WL_CONNECTED);       // 接続に成功するまで待つ
    disp("janken88","Example 12 Janken");       // 持ち手＋タイトルを表示
    if(USER == "") USER = String(SSID).substring(String(SSID).length()-4);
}

void loop(){                                    // 繰り返し実行する関数
    int jan = 8;                                // ユーザの手(未取得時=8)
    int ken = 8;                                // クラウド側の手(未取得時=8)

    M5.update();                                // ボタン情報を更新
    delay(10);    // 誤作動防止(参考文献 github.com/m5stack/M5Stack/issues/52 )
    if(M5.BtnA.wasPressed()) jan = 0;           // ボタンAのときはグー(0本指)
    if(M5.BtnB.wasPressed()) jan = 2;           // ボタンBのときはチョキ(2本指)
    if(M5.BtnC.wasPressed()) jan = 5;           // ボタンCのときはパー(5本指)
    if(jan == 8) return;                        // ボタン無押下時にloopの先頭へ
    disp("janken" + String(jan), "Shoot!");     // 変数janに応じた表示

    /* クラウドへの接続処理 */
    String S = String(URL);                     // HTTPリクエスト用の変数
    S += "?user=" + USER;                       // ユーザ名のクエリを追加
    S += "&throw="+ String(jan);                // ジャンケンの手を追加
    WiFiClientSecure client;                    // TLS/TCP/IP接続部の実体を生成
    client.setCACert(rootCACertificate);        // ルートCA証明書を設定
    HTTPClient https;                           // HTTP接続部の実体を生成
    https.begin(client, S);                     // 初期化と接続情報の設定
    int httpCode = https.GET();                 // HTTP接続の開始
    S = https.getString();                      // 受信結果を変数Sへ代入
    // Serial.println(httpCode);                // httpCodeをシリアル出力
    // Serial.println(S);                       // 受信結果をシリアル出力
    if(httpCode == 200){                        // HTTP接続に成功したとき
        ken = S.substring(S.indexOf("\"net\":")+13).toInt();       // パースken
        rate = S.substring(S.indexOf("\"win rate\":")+12).toInt(); // パースrate
    }                                           // パース方法は受信サンプル参照
    https.end();                                // HTTPクライアントの処理を終了
    client.stop();                              // TLS(SSL)通信の停止

    /* 勝敗判定・表示処理 */
    String msg = "Draw";                        // 変数msgに「引き分け」を代入
    if((jan / 2 + 1) % 3 == (ken / 2)){         // ユーザの方が強い手のとき
        msg = "You Win";                        // 「勝ち」
    }else if(jan != ken){                       // 勝ちでも引き分けでも無いとき
        msg = "You Lose";                       // 「負け」
    }
    if(rate) msg += ", rate=" + String(rate);   // 勝率をmsgへ追加
    disp("janken"+String(jan)+String(ken),msg); // 画像と勝敗表示を更新
}

/*
受信サンプル：
{
    "statusCode": 200, "body":
    {
        "status": "ok", "message": "test won me",
        "you": ["C", 2, "scissors", "\u30c1\u30e7\u30ad"],
        "net": ["P", 5, "paper", "\u30d1\u30fc"],
        "win rate": 50
    }
}

簡易パーサ(ken) indexOf("\"net\":")+13
        "net": ["P", 5, "paper", "\u30d1\u30fc"],
        01234567890123
}

簡易パーサ(rate) indexOf("\"win rate\":")+12
        "win rate": 50
        0123456789012
*/
