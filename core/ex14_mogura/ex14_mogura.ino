/*******************************************************************************
Example 14 mogura for M5Stack ～ シンプル もぐらたたき ゲーム～

                                               Copyright (c) 2022 Wataru KUNINO
*******************************************************************************/

#include <M5Stack.h>                            // M5Stack用ライブラリの組込
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiClientSecure.h>                   // TLS(SSL)通信用ライブラリ
#include <HTTPClient.h>                         // HTTP通信用ライブラリ
#include "root_ca.h"                            // HTTPS通信用ルート証明書
#define SSID "1234ABCD"                         // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
#define URL  "https://bokunimo.com/mogura/"     // クラウドサービスのURL

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

int mogura[3]={0,0,0};                          // モグラの上下位置(0～3)
int pt = 0;                                     // 得点

int getRank(int pt){                            // 得点の順位を取得
    int rank = 0;                               // 順位を保持する関数
    String S = String(URL);                     // HTTPリクエスト用の変数
    S += "?user=" + USER;                       // ユーザ名のクエリを追加
    S += "&game=mogura";                        // ゲーム名をクエリに追加
    if(pt) S += "&p="+ String(pt);              // 点数ptとその値を追加
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
        rank = S.substring(S.indexOf("\"rank\":")+8).toInt(); // パースrank
    }                                           // パース方法は受信サンプル参照
    https.end();                                // HTTPクライアントの処理を終了
    client.stop();                              // TLS(SSL)通信の停止
    return rank;                                // 順位を応答
}

void dispText(String msg){                      // LCDに文字表示する
    drawJpgHeadFile("mogura5",0,111);           // 文字を消去
    M5.Lcd.setTextColor(WHITE);                 // テキスト文字の色を設定(白)
    for(int x=-2; x<=2; x+=2) for(int y=-2; y<=2; y+=2) if(x||y){
        M5.Lcd.drawCentreString(msg,160+x,113+y,4);
    }                                           // テキストの背景を描画する
    M5.Lcd.setTextColor(0);                     // テキスト文字色を設定(黒)
    M5.Lcd.drawCentreString(msg,160,113,4);     // 文字列を表示
}

void setup(){                                   // 一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    drawJpgHeadFile("mogura");                  // 顔を表示
    dispText("Example 14 Mogura");              // タイトルを表示
    delay(3000);                                // 3秒間の待ち時間処理
    dispText("GAME START");                     // ゲーム開始を表示
    delay(1000);                                // 3秒間の待ち時間処理
    dispText("");                               // 文字を消去
}

void loop(){
    for(int i=0; i<3; i++){                     // 繰り返し実行する関数
        mogura[i] += random(4) - 2;             // モグラを乱数で上下(+1/-2)
        if(mogura[i] < 0 ) mogura[i] = 0;       // 0未満の時に0を代入
        if(mogura[i] > 3 ) mogura[i] = 3;       // 3超過の時に3を代入
        drawJpgHeadFile("mogura"+String(mogura[i]), 106*i, 140); // モグラを表示
        M5.update();                            // ボタン情報を更新
        delay(33);                              // 待ち時間処理
        byte btn = M5.BtnA.wasPressed();        // 左ボタン(A)の状態を取得
        btn += 2 * M5.BtnB.wasPressed();        // 中央ボタン(B)の状態を取得
        btn += 4 * M5.BtnC.wasPressed();        // 右ボタン(b)の状態を取得
        for(int k=0; k<3; k++){                 // 各モグラについて
            if((mogura[k] > 1)&&((btn>>k)&1) ){ // 顔が出ているモグラをHit
                int p = 1 + (mogura[k]-2)*9;    // 得点を計算
                dispText("Hit! +"+String(p));   // 取得点数を表示
                pt += p;                        // 合計得点を計算
                drawJpgHeadFile("mogura4", 106*k, 140); // 叩かれたモグラを表示
                delay(1000);                    // 1秒間の待ち時間処理
                dispText(String(pt)+" pt");     // スコア表示
                mogura[k]=0;
            }
        }
    }
}

void end(){
    int rank = getRank(int pt)
    dispText("Rank = "+String(rank)+" "+String(pt)+" pt");     // スコア表示
    M5.Lcd.drawCentreString("GAME START",256,224,2); // 文字列を表示
    do{                                         // ボタンが押されるまで待機する
        M5.update();                            // ボタン情報を更新
        delay(10);                              // 待ち時間処理
    }while(!M5.BtnC.wasPressed());              // ボタンが押されるまで繰り返す
}

/*
受信サンプル：
{"statusCode": 200, "body": {"status": "ok", "rank": 6}}

簡易パーサ(n) indexOf("\"n\":")+5
        "rank": 6
        01234567890123
*/
