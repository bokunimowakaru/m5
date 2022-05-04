/*******************************************************************************
Example 7: GPS(GNSS)の位置情報を取得し、Wi-Fiで送信する
（動作確認済みGPSモジュール：u-blox NEO-6M NEO-6M-0-001, 杭州中科微电子 AT6558）
・約30秒ごとに位置情報をWi-Fi送信します。
・ボタン操作で3つの動作モードを切り替えることができます。
・左ボタンで、日本地図上に現在位置を表示します。
・中央ボタンで、現在地を中心にした相対位置表示モードになります。
・右ボタンで、ログ表示モードになります。
  （データの一部をLCD表示すすとともに全データをシリアルとUDPで出力します。）

    使用機材(例)：M5Stack Core + Mini GPS/BDS Unit (AT6558)

                                               Copyright (c) 2022 Wataru KUNINO
********************************************************************************
★ご注意★・屋外での視覚用にLCDの★輝度を最大に設定★してあります。
          ・GPS Unit を Grove互換端子に接続してください。
          ・GPS Unitの電源を入れてから位置情報が得られるまで数分以上を要します。
*******************************************************************************/

#include <M5Stack.h>                            // M5Stack用ライブラリの組み込み

#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ
#include "lib_TinyGPS.h"                        // GPS通信用ライブラリ
#include "japan_jpg.h"                          // 日本地図のJPEGデータ

#define SSID "1234ABCD"                         // 無線LANアクセスポイント SSID
#define PASS "password"                         // パスワード
#define PORT 1024                               // 送信のポート番号
#define DEVICE "gns_s_3,"                       // デバイス名(5字+"_"+番号+",")
#define LOGUDP "log_s_0,"                       // デバイス名(5字+"_"+番号+",")
RTC_DATA_ATTR int mode = 0;                     // 0:日本地図 1:座標表示 2:Raw
String mode_S[3] = {"Map","Chart","Raw"};       // モード表示用の配列変数

/*******************************************************************************
 GPS位置と地図画像の位置座標対応表
 *******************************************************************************
 ・位置情報と画像ファイルの位置座標との対応表です。
 ・測定点の近い座標を追加することで、より精度の高い表示が可能になります。
 ・(近郊の地図や海外など)地図画像を変更する場合は、書き換えが必要です。

 要素数：JpMAP_N
 書式：{経度,緯度,X座標,Y座標}
*******************************************************************************/

#define JpMAP_N 7                               // 位置座標対応表の件数
const float japan[JpMAP_N][4]={
    {129.87, 32.76,  21, 194},
    {135.00, 34.65, 104, 178},
    {139.74, 35.66, 176, 173},
    {139.15, 37.83, 171, 133},
    {141.92, 45.50, 231,  3},
    {127.65, 26.20, 198, 234},
    {128.30, 26.83, 214, 220}
};                                              // 位置座標対応データ

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

TinyGPS gps;                                    // GPSライブラリのインスタンス
float lat_origin =  35.+40./60.+37.8311/3600.;  // 相対位置表示の初期値 緯度
float lon_origin = 139.+44./60.+51.5282/3600.;  // 相対位置表示の初期値 経度
float alt_origin = 30.300;                      // 相対位置表示の初期値 標高
float lat, lon, alt;                            // 緯度,経度,標高データ保持用
boolean gps_avail = false;                      // GPSデータの有無
boolean trig = false;                           // 送信用トリガ
unsigned long base_ms = 0;                      // センサ検知時の時刻

#define LCD_ORIGIN_Y 16                         // 表示開始位置
#define LCD_LINES   (240 - LCD_ORIGIN_Y)/8      // 表示可能な行数

int lcd_line = 0;                               // 現在の表示位置(行)
char lcd_buf[LCD_LINES][54];                    // LCDスクロール表示用バッファ

void lcd_log(String S){                         // LCDにログを表示する関数
    if(!lcd_line) M5.Lcd.println("Logging:");   // 先頭行にLoging...表示
    lcd_line++;                                 // 次の行に繰り上げる
    if(lcd_line >= LCD_LINES){                  // 行末を超えたとき
        lcd_line = LCD_LINES - 1;               // 行末に戻す
        for(int y=1;y<LCD_LINES;y++){           // 1行分、上にスクロールする処理
            int org_y = 8*(y-1)+LCD_ORIGIN_Y;   // 文字を表示する位置を算出
            int sLenX = strlen(lcd_buf[y]) * 6; // 消去する文字のX座標
            M5.Lcd.fillRect(sLenX, org_y, 320-sLenX, 8, BLACK); // 文字を消去
            M5.Lcd.setCursor(0, org_y);         // 描画する文字の座標を指定
            M5.Lcd.print(lcd_buf[y]);           // バッファ内のデータを表示する
            memcpy(lcd_buf[y-1],lcd_buf[y],54); // バッファ内データを1行分移動
        }
    }
    int sLenX = S.length() * 6;                 // 消去する文字のX座標
    M5.Lcd.fillRect(sLenX, 8*lcd_line+LCD_ORIGIN_Y, 320 - sLenX, 8, BLACK);
    M5.Lcd.setCursor(0, 8*lcd_line+LCD_ORIGIN_Y); // 現在の文字表示位置に移動
    M5.Lcd.print(S.substring(0,53));            // 53文字までをLCDに表示
    S.toCharArray(lcd_buf[lcd_line],54);        // 配列型変換(最大長時\0付与有)
}

String ip2s(uint32_t ip){                       // IPアドレスを文字列に変換
    String S;                                   // 文字列変数を生成
    for(int i=3;i>=0;i--){                      // forループ4バイト分の処理
        S += String((ip>>(8*ip))%256);          // 1バイトずつ数値を文字列に追加
        if(i) S += ".";                         // 区切り文字(ピリオド)の追加
    }                                           // forの繰り返し(4バイト分)
    return S;                                   // 文字列を応答
}

void lcd_head(){                                // 各種の状態をLCD上部に表示
    M5.Lcd.setCursor(0,0);                      // 文字表示位置を0,0に
    M5.Lcd.print("M5 GPS, ");                   // 「M5 GNSS」をLCD表示
    String S[2]={"No","Yes"};                   // No/Yesを表示するための変数
    M5.Lcd.print(mode_S[mode] + " Val=" + S[(int)gps_avail] + " "); // Mode表示
    if(WiFi.status() == WL_CONNECTED) M5.Lcd.print(WiFi.localIP()); // IP表示
    else M5.Lcd.print("WiFi=" + String(WiFi.status()));             // Wi-Fi表示
    M5.Lcd.printf("      \nlat=%.6f, lon=%.6f  \n",lat,lon);        // 位置表示
}

void lcd_cls(int mode){                         // LCDを消去して基本画面を描画
    switch(mode){                               // 画面モードに応じた処理
        case 0:                                 // mode=0のとき：
            M5.Lcd.drawJpg(japan_jpg,japan_jpg_len); // LCDに日本地図を表示
            // M5.Lcd.drawJpgFile(SD, "/japan.jpg"); // LCDにjapan.jpgを表示
            // M5.Lcd.setTextColor(BLACK,WHITE);     // 文字色を黒(背景白)に設定
            M5.Lcd.setTextColor(WHITE,BLACK);   // 文字色を白(背景なし)に設定
            break;                              // switch処理を終了
        case 1:                                 // mode=1のとき
            M5.Lcd.fillScreen(BLACK);           // 表示内容を消去
            M5.Lcd.setTextColor(WHITE,BLACK);   // 文字色を白(背景なし)に設定
            M5.Lcd.drawLine(0, 120, 320, 120, LIGHTGREY);   // 水平線を描画
            M5.Lcd.drawLine(160, 0, 160, 240, LIGHTGREY);   // 垂直線を描画
            M5.Lcd.drawCircle(160,120,100,LIGHTGREY);       // 円を描画
            if(gps_avail){                      // GNSS情報があるとき
                lat_origin = lat;               // 現在のGNSS位置を保持(緯度)
                lon_origin = lon;               // 現在のGNSS位置を保持(経度)
                alt_origin = alt;               // 現在のGNSS位置を保持(高度)
            }
            break;                              // switch処理を終了
        case 2:                                 // mode=2のとき
            M5.Lcd.fillScreen(BLACK);           // 表示内容を消去
            M5.Lcd.setTextColor(WHITE,BLACK);   // 文字色を白(背景黒)に設定
            break;                              // switch処理を終了
    }
    lcd_head();                                 // 現在の状態をLCD上部に表示
    lcd_line = 0;                               // 現在の表示位置を保持
}

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    M5.Lcd.setBrightness(255);                  // LCD輝度を最大に設定
    lcd_cls(mode);                              // 画面を消去する関数を実行
    setupGps();                                 // GPS初期化
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
}

void loop(){                                    // 繰り返し実行する関数
    M5.update();                                // ボタン状態の取得
    int btn=M5.BtnA.wasPressed()+2*M5.BtnB.wasPressed()+4*M5.BtnC.wasPressed();
    switch(btn){                                // ボタンに応じた処理
        case 1: mode = 0; lcd_cls(mode); break; // 0:日本地図
        case 2: mode = 1; lcd_cls(mode); break; // 1:座標表示
        case 4: mode = 2; lcd_cls(mode); break; // 2:Raw + UDP送信
        default: btn = 0; break;
    }

    if(mode == 0){                              // 地図表示モードのとき
        gps_avail = getGpsPos(gps,&lat,&lon,&alt);  // GPSから位置情報を取得
        if(gps_avail){                          // GNSS情報が得られたとき
            float min[2]={999,999};             // 検索結果の保持用(最小値)
            int ind[2]={0,0};                   // 検索結果の保持用(配列番号)
            for(int i = 0; i < JpMAP_N; i++){   // 位置座標対応表を検索
                float d0 = lon - japan[i][0];   // 経度の差をd0に
                float d1 = lat - japan[i][1];   // 緯度の差をd1に
                float d = sqrt(pow(d0,2)+pow(d1,2)); // d0とd1から距離を計算
                if(min[0] < d && d < min[1]){   // 過去の結果の2位よりも近い
                    min[1] = d;                 // 暫定2位として距離を更新
                    ind[1] = i;                 // 暫定2位として配列番号を更新
                }else if(d < min[0]){           // 過去の結果の1位よりも近い
                    min[1] = min[0];            // 現1位の距離を2位に更新
                    ind[1] = ind[0];            // 現1位の配列番号を2位に更新
                    min[0] = d;                 // 暫定1位として距離を更新
                    ind[0] = i;                 // 暫定1位として配列番号を更新
                }
            }                                   // 全位置座標対応表の繰り返し
            int x=(int)(
                (japan[ind[1]][2]-japan[ind[0]][2])
                *((lon-japan[ind[0]][0])/(japan[ind[1]][0]-japan[ind[0]][0]))
                +japan[ind[0]][2]
            );                                  // 1位と2位の結果からX座標を計算
            int y=(int)(
                (japan[ind[1]][3]-japan[ind[0]][3])
                *((lat-japan[ind[0]][1])/(japan[ind[1]][1]-japan[ind[0]][1]))
                +japan[ind[0]][3]
            );                                  // 1位と2位の結果からY座標を計算
            if(x>=0 && x<320 && y>=0 && y<240){ // 計算結果が表示領域内のとき
                M5.Lcd.fillCircle(x,y,3,RED);   // 赤色の丸印を描画
                M5.Lcd.drawCircle(x,y,4,WHITE); // 白色の縁取りを描画
            }
            lcd_head();                         // 現在の状態をLCD上部に表示
            M5.Lcd.println("alt="+String(alt,0)+"m "); // 高度を表示
            // M5.Lcd.printf("x=%d, y=%d, i0=%d, i1=%d\n",x,y,ind[0],ind[1]);
        }
    }else if(mode == 1){                        // 座標表示モード
        gps_avail = getGpsPos(gps,&lat,&lon,&alt);  // GPSから位置情報を取得
        if(gps_avail){
            float x =  9055. * (lon - lon_origin);
            float y = 11095. * (lat - lat_origin);
            if(x>-160 && x<160 && y>-120 && y<120){
                M5.Lcd.fillCircle(160+(int)x,120-(int)y,3,RED);
            }
            lcd_head();
            M5.Lcd.println("alt="+String(alt,0)+"m ");
            M5.Lcd.println("dis="+String(sqrt(pow(x,2)+pow(y,2))/100,3)+"km ");
        }
    }else if(mode == 2){                        // Raw表示 + UDP送信
        char s[128];                            // 最大文字長127文字
        boolean i = getGpsRawData(s,128);       // GPS生データを取り込む
        if(s[0] != 0){                          // データがあるとき
            Serial.println(s);                  // シリアルに出力
            if(strstr(s,gps.GPS_TERM_NAMES[3])){ // "GNGGA"が含まれるとき
                lcd_log(s);                     // 位置情報のときにLCDに表示
            }
            if(WiFi.status() == WL_CONNECTED){  // Wi-Fi接続状態の時
                WiFiUDP udp;                    // UDP通信用のインスタンス定義
                udp.beginPacket(UDPTO_IP, PORT);  // UDP送信先を設定
                udp.print(LOGUDP);              // ログ用ヘッダを送信
                udp.println(s);                 // 値を送信
                udp.endPacket();                // UDP送信の終了(実際に送信)
                delay(10);                      // 送信待ち時間
                udp.stop();                     // UDP通信の終了
            }
        }
        if(millis() - base_ms > 30000){
            gps_avail = getGpsPos(gps,&lat,&lon,&alt);  // GPSから位置情報を取得
            lcd_head();
        }
    }

    if(!trig && millis() - base_ms > 30000){    // 30秒以上経過
        base_ms = millis();
        trig = true;
        if(WiFi.status() != WL_CONNECTED){
            WiFi.begin(SSID,PASS);              // 無線LANアクセスポイント接続
            lcd_head();
        }
    }

    if(gps_avail && trig && WiFi.status() == WL_CONNECTED){
        if(alt>1000) alt = 0.;                  // 測定不可時の対応
        String S = String(DEVICE);              // 送信データ保持用の文字列変数
        S += String(lat,6) + ", ";              // 緯度値を送信データに追記
        S += String(lon,6) + ", ";              // 経度値を送信データに追記
        S += String(alt,0);                     // 標高値を送信データに追記
        if(mode==2) lcd_log("UDP: " + S);
        else M5.Lcd.println("UDP: sended");     // LCDに表示

        WiFiUDP udp;                            // UDP通信用のインスタンス定義
        udp.beginPacket(UDPTO_IP, PORT);        // UDP送信先を設定
        udp.println(S);                         // センサ値を送信
        udp.endPacket();                        // UDP送信の終了(実際に送信)
        delay(10);                              // 送信待ち時間
        udp.stop();                             // UDP通信の終了

        if(strcmp(Amb_Id,"00000") != 0){        // Ambient設定時
            S = "{\"writeKey\":\""+String(Amb_Key); // (項目)writeKey,(値)ライトキー
            S+= "\",\"d1\":\"" + String(alt,0); // (項目)d1,(値)標高値
            S+= "\",\"lat\":\"" + String(lat,6); // (項目)lat,(値)緯度値
            S+= "\",\"lng\":\"" + String(lon,6); // (項目)lng,(値)経度値
            S+= "\"}";
            HTTPClient http;                    // HTTPリクエスト用インスタンス
            http.setConnectTimeout(15000);      // タイムアウトを15秒に設定する
            String url="http://ambidata.io/api/v2/channels/"+String(Amb_Id)+"/data";
            http.begin(url);                    // HTTPリクエスト先を設定する
            http.addHeader("Content-Type","application/json"); // JSON形式を設定する
            http.POST(S);                       // GPS情報をAmbientへ送信する
            http.end();                         // HTTP通信を終了する
            if(mode==2) lcd_log("HTTP: " + url);
            else M5.Lcd.println("HTTP: sended"); // LCDに表示
        }
        if(mode != 2) WiFi.disconnect();        // Wi-Fiの切断
        trig = false;
    }
}

/*******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/m5stickc_plus/arduino

M5StickC Arduino Library API 情報 (旧モデル M5StackC 用)：
https://docs.m5stack.com/en/api/stickc/system_m5stickc

GPSのNMEAフォーマット(hiramine.com)
https://www.hiramine.com/physicalcomputing/general/gps_nmeaformat.html

【引用コード】
https://github.com/bokunimowakaru/SORACOM-LoRaWAN/blob/master/examples/cqp_ex05_gps_bin
*******************************************************************************/