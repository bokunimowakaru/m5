/*******************************************************************************
Example 13 daruma for M5Stack ～PIR Sensor ユニットで だるまさんがころんだ～

                                               Copyright (c) 2022 Wataru KUNINO
*******************************************************************************/

#include <M5Stack.h>                            // M5Stack用ライブラリの組み込み
#define PIN_PIR 22                              // G22にセンサ(人感/ドア)を接続
RTC_DATA_ATTR uint32_t pir_delay = 2000;        // 人感センサの解除遅延時間ms(2秒)

boolean pir;                                    // 人感センサ状態
int bar100 = 0;                                 // 前回の棒グラフ値(100分率)
int barPrev = 0;                                // 前回の棒グラフ値(ピクセル数)

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

void disp(String filename, String msg=""){      // LCDにJPEGファイルを表示する
    drawJpgHeadFile(filename);                  // filenameに応じた画像をLCD表示
    M5.Lcd.setTextColor(WHITE);                 // テキスト文字の色を設定(白)
    for(int x=-2; x<=2; x+=2) for(int y=-2; y<=2; y+=2) if(x||y){
        M5.Lcd.drawCentreString(msg,160+x,120+y,4);
    }                                           // テキストの背景を描画する
    M5.Lcd.setTextColor(0);                     // テキスト文字の色を設定(黒)
    M5.Lcd.drawCentreString(msg,160,120,4);     // 文字列を表示
    barPrev = 0;                                // 棒グラフが消えるのでリセット
}

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    pinMode(PIN_PIR,INPUT);                     // センサ接続したポートを入力に
    disp("daruma3","Example 13 Daruma-san");    // 持ち手＋タイトルを表示
    disp("daruma2");                            // 顔を表示
    disp("daruma4");                            // タイトルを表示
    while(digitalRead(PIN_PIR));                // 非検出状態になるまで待つ
    delay(3000);                                // 3秒間の待ち時間処理
    disp("daruma3","GAME START");               // ゲーム開始
    delay(1000);                                // 1秒間の待ち時間処理
}

void loop(){                                    // 繰り返し実行する関数
    disp("daruma0");                            // 顔を表示
    M5.Lcd.drawCentreString("BREAK",64,224,2);  // 文字列"BREAK"を表示
 	for(int i=0; i <= 100; i++){                // 棒グラフを増加させる
        M5.update();                            // ボタン情報を更新
        dispBar(i);                             // 棒グラフの描画
        if(M5.BtnA.isPressed()){                // Aボタンが押されたとき
            M5.Lcd.fillRect(0,224,128,16,WHITE); // "BREAK"を消去
            disp("daruma2","Cleared!");         // 顔を表示
            end();                              // 終了関数endを実行
            return;                             // loop関数の先頭に戻る
        }
        if(i==33){                              // 棒グラフ33%のとき
            disp("daruma4");                    // タイトルを表示
        }
    }
    M5.Lcd.fillRect(0,224,128,16,WHITE);        // "BREAK"を消去
    disp("daruma2");                            // 顔を表示
    delay(pir_delay);                           // PIRセンサの遅延分の待ち時間
	for(int i=100; i >= 0; i--){                // 棒グラフを減らす処理
        dispBar(i);                             // 棒グラフの描画
        pir = digitalRead(PIN_PIR);             // 人感センサ値を取得
        if(pir){                                // センサ反応時
            disp("daruma2","Failed");           // "Failed"を表示
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
