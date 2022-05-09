/*******************************************************************************
Example 14 mogura for M5Stack ～ シンプル もぐらたたき ゲーム～

                                               Copyright (c) 2022 Wataru KUNINO
*******************************************************************************/

#include <M5Stack.h>                                // M5Stack用ライブラリの組込

int mogura[3]={0,0,0};                              // モグラの位置
int pt = 0;                                         // 得点

void dispText(String msg){                          // LCDに文字表示する
    drawJpgHeadFile("mogura5",0,111);               // 文字を消去
    M5.Lcd.setTextColor(WHITE);                     // テキスト文字の色を設定(白)
    for(int x=-2; x<=2; x+=2) for(int y=-2; y<=2; y+=2) if(x||y){
        M5.Lcd.drawCentreString(msg,160+x,113+y,4);
    }                                               // テキストの背景を描画する
    M5.Lcd.setTextColor(0);                         // テキスト文字色を設定(黒)
    M5.Lcd.drawCentreString(msg,160,113,4);         // 文字列を表示
}

void setup(){                                       // 一度だけ実行する関数
    M5.begin();                                     // M5Stack用ライブラリの起動
    drawJpgHeadFile("mogura");                      // 顔を表示
    dispText("Example 14 Mogura");                  // タイトルを表示
    delay(3000);                                    // 3秒間の待ち時間処理
    dispText("GAME START");                         // ゲーム開始を表示
    delay(1000);                                    // 3秒間の待ち時間処理
    dispText("");                                   // 文字を消去
}

void loop(){
    for(int i=0; i<3; i++){                     // 繰り返し実行する関数
        mogura[i] += random(4) - 2;             // モグラを(乱数で)上下
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
                drawJpgHeadFile("mogura4", 106*k, 140);
                delay(1000);                    // 1秒間の待ち時間処理
                dispText(String(pt)+" pt");     // スコア表示
                mogura[k]=0;
            }
        }
    }
}

void end(){
    M5.Lcd.drawCentreString("GAME START",256,224,2); // 文字列を表示
    do{                                             // ボタンが押されるまで待機する
        M5.update();                                // ボタン情報を更新
        delay(10);                                  // 待ち時間処理
    }while(!M5.BtnC.wasPressed());                  // ボタンが押されるまで繰り返す
}
