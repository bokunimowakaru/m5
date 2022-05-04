/*******************************************************************************
Example 7: GPS(GNSS)の位置情報を取得する
（動作確認済みGPSモジュール：u-blox NEO-6M NEO-6M-0-001, 杭州中科微电子 AT6558）
    使用機材(例)：M5Stack Core + Mini GPS/BDS Unit (AT6558)

                                               Copyright (c) 2022 Wataru KUNINO
********************************************************************************
★ご注意★・屋外での視覚用にLCDの★輝度を最大に設定★してあります。
          ・GPS Unit を Grove互換端子に接続してください。
          ・GPS Unitの電源を入れてから位置情報が得られるまで数分以上を要します。
*******************************************************************************/

#include <M5Stack.h>                            // M5Stack用ライブラリの組み込み
#include "lib_TinyGPS.h"                        // GPS通信用ライブラリ
#include "japan_jpg.h"                          // 日本地図のJPEGデータ
RTC_DATA_ATTR int mode = 0;                     // 0:日本地図 1:座標表示 2:Raw

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

TinyGPS gps;                                    // GPSライブラリのインスタンス
float lat, lon, alt;                            // 緯度,経度,標高データ保持用
boolean gps_avail = false;                      // GPSデータの有無

void lcd_cls(int mode){                         // LCDを消去して基本画面を描画
    switch(mode){                               // 画面モードに応じた処理
        case 0:                                 // mode=0のとき：
            M5.Lcd.drawJpg(japan_jpg,japan_jpg_len); // LCDに日本地図を表示
            M5.Lcd.setTextColor(WHITE,BLACK);   // 文字色を白(背景なし)に設定
            break;                              // switch処理を終了
    }
}

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    M5.Lcd.setBrightness(255);                  // LCD輝度を最大に設定
    lcd_cls(mode);                              // 画面を消去する関数を実行
    setupGps();                                 // GPS初期化
}

void loop(){                                    // 繰り返し実行する関数
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
        }
    }
}

/*******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/m5core/arduino

M5Stack Arduino Library API 情報：
https://docs.m5stack.com/en/api/core/system

GPSのNMEAフォーマット(hiramine.com)
https://www.hiramine.com/physicalcomputing/general/gps_nmeaformat.html

【引用コード】
https://github.com/bokunimowakaru/SORACOM-LoRaWAN/blob/master/examples/cqp_ex05_gps_bin
*******************************************************************************/
