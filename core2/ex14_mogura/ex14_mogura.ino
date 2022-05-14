/*******************************************************************************
Example 14 mogura for M5Stack Core2 ～ シンプル もぐらたたき ゲーム～

                                               Copyright (c) 2022 Wataru KUNINO
*******************************************************************************/

#include <M5Core2.h>                            // M5Stack用ライブラリ組み込み
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiClientSecure.h>                   // TLS(SSL)通信用ライブラリ
#include <HTTPClient.h>                         // HTTP通信用ライブラリ
#include "root_ca.h"                            // HTTPS通信用ルート証明書
#define SSID "1234ABCD"                         // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
#define URL  "https://bokunimo.com/score/"      // クラウドサービスのURL

/*******************************************************************************
下記の USER にニックネームなどを入力してください。※スペースは使用できません。
*******************************************************************************/

String USER = "";                               // クラウドへ送信するユーザ名

/*******************************************************************************
ユーザ名 USER は、半角英文字と数字、8文字までです。※スペースは使用できません。
例） String USER = "wataru";
********************************************************************************
送信データ（ユーザ名、IPアドレス等）はサーバ内に保存し、下記の目的で使用します。
ユーザ名はなるべく実名を避け、ニックネームなどを使用してください。
ユーザ名はHTTPSを使用し、送信先のサーバの認証と暗号化を講じて送信します。

【利用目的・用途】
・本サービスのための端末の特定
・送信データの傾向学習用
・システム・アルゴリズムの改良
・ランキング表示や対戦時の表示用
・その他、ネット・サービスの開発用

【ご注意】
・当方の過失による流出については責任を負いません
・データ消失については責任を負いません（誤った削除依頼で消失する場合もあります）
・削除依頼については対応しますが、適正な依頼かどうかは当方の判断に基づきます
・プログラムの改変によって、認証など所定の動作が行われなくなる場合があります
・アクセス回数やアクセス頻度によって、所定の動作が行われなくなる場合があります
・法令や社会倫理に基づく情報開示の要請に対し、当方の判断で応じる場合があります

本プログラムを機器に書き込んだ時点で、同意いただけたものとします。
*******************************************************************************/

int mogura[3]={0,0,0};                          // モグラの上下位置(0～3)
int pt = 0;                                     // 得点
unsigned long start_ms = 0;                     // ゲーム開始時刻

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
        /* (参考) Arduino_JSON.h ベータ版パース方法(要#include <Arduino_JSON.h>)
        JSONVar json = JSON.parse(S);           // JSON型のオブジェクトに変換
        rank = json["body"]["rank"];            // body内のrankを抽出
        */
    }                                           // パース方法は受信サンプル参照
    https.end();                                // HTTPクライアントの処理を終了
    client.stop();                              // TLS(SSL)通信の停止
    return rank;                                // 順位を応答
}

void getRankList(){                             // Top10リストを取得
    M5.Lcd.fillRect(0, 0, 320, 240, WHITE);     // 画面を消去
    String S = String(URL);                     // HTTPリクエスト用の変数
    S += "?game=mogura&list";                   // リスト取得
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
        int p1=0, p2, p3=0;                     // 文字位置の保持用
        String unit[] = {"st","nd","rd","th"};  // 単位
        for(int i=0;i<10;i++){                  // Top10リスト表示
            M5.Lcd.drawCentreString(String(i+1)+unit[i<4?i:3],32,i*24,4);
            p1 = S.indexOf("\"id\":",p1) + 7;   // ユーザ名の文字位置を検索
            p2 = S.indexOf("\"",p1);            // ユーザ名の終了位置を検索
            M5.Lcd.drawString(S.substring(p1,p2),64,i*24,4); // ユーザ名を表示
            p3 = S.indexOf("\"score\":",p3)+9;  // 得点の文字位置を検索
            int p = S.substring(p3).toInt();    // 得点を取得
            M5.Lcd.drawRightString(String(p)+" pt",316,i*24,4);
        }
    }                                           // パース方法は受信サンプル参照
    https.end();                                // HTTPクライアントの処理を終了
    client.stop();                              // TLS(SSL)通信の停止
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
    drawJpgHeadFile("mogura");                  // ゲーム画面をを表示
    dispText("Example 14 Mogura");              // タイトルを表示
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    while(WiFi.status() != WL_CONNECTED);       // 接続に成功するまで待つ
    dispText("GAME START");                     // ゲーム開始を表示
    delay(1000);                                // 3秒間の待ち時間処理
    dispText("");                               // 文字を消去
    start_ms = millis();                        // ゲーム開始時刻を変数に保持
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
    if(millis() - start_ms >= 30000){           // ゲーム開始から30秒以上が経過
        dispText("GAME OVER");                  // ゲームオーバー表示
        delay(1000);                            // 1秒間の待ち時間処理
        end();                                  // ゲーム終了
    }
}

void end(){
    int rank = getRank(pt);
    if(rank>0){
        dispText("Rank = "+String(rank)+", Score = "+String(pt)+" pt"); // スコア表示
    }else{
        dispText(String(pt)+" pt"); // スコア表示
    }
    M5.Lcd.drawCentreString("Top 10",160,224,2); // 文字列を表示
    M5.Lcd.drawCentreString("GAME START",256,224,2); // 文字列を表示
    do{                                         // ボタンが押されるまで待機する
        M5.update();                            // ボタン情報を更新
        delay(10);                              // 待ち時間処理
        if(M5.BtnB.wasPressed()){               // 中央ボタンが押されたとき
            getRankList();                      // Top10リストの表示を実行
            delay(1000);                        // 簡易的なチャタリング防止
        }
    }while(!M5.BtnC.wasPressed());              // 右ボタンが押されるまで
    drawJpgHeadFile("mogura");                  // ゲーム画面をを表示
    pt = 0;                                     // 得点のリセット
    start_ms = millis();                        // ゲーム開始時刻を変数に保持
}

/*
受信サンプル：
{"statusCode": 200, "body": {"status": "ok", "rank": 6}}

簡易パーサ(n) indexOf("\"n\":")+5
        "rank": 6
        01234567890123

Top10リスト：
{"statusCode": 200, "body": {"status": "ok", "rank": 0, "list": [{"id": "BOKU", "score": 12}, {"id": "BOKU", "score": 10}, {"id": "BOKU", "score": 10}, {"id": "BOKU", "score": 9}, {"id": "BOKU", "score": 8}, {"id": "BOKU", "score": 7}, {"id": "BOKU", "score": 7}, {"id": "BOKU", "score": 7}, {"id": "BOKU", "score": 7}, {"id": "BOKU", "score": 7}]}}

簡易パーサ(id) indexOf("\"id\":")+7
        "id": "BOKU"
        01234567890123

簡易パーサ(score) indexOf("\"score\":")+7
        "score": 12
        01234567890123

*/
