/*******************************************************************************
Example 14 mogura for M5Stack ～ シンプル もぐらたたき ゲーム～

                                               Copyright (c) 2022 Wataru KUNINO
*******************************************************************************/

#include <M5Stack.h>                            // M5Stack用ライブラリの組み込み

int mogura[3]={0,0,0};

void disp(String filename, String msg=""){      // LCDにJPEGファイルを表示する
    drawJpgHeadFile(filename);                  // filenameに応じた画像をLCD表示
    M5.Lcd.setTextColor(WHITE);                 // テキスト文字の色を設定(白)
    for(int x=-2; x<=2; x+=2) for(int y=-2; y<=2; y+=2) if(x||y){
        M5.Lcd.drawCentreString(msg,160+x,120+y,4);
    }                                           // テキストの背景を描画する
    M5.Lcd.setTextColor(0);                     // テキスト文字の色を設定(黒)
    M5.Lcd.drawCentreString(msg,160,120,4);     // 文字列を表示
}

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    disp("mogura","Example 14 Mogura");         // モグラ＋タイトルを表示
    delay(3000);                                // 3秒間の待ち時間処理
    disp("mogura","GAME START");                // ゲーム開始
    delay(1000);                                // 1秒間の待ち時間処理
    disp("mogura");                             // 顔を表示
}

void loop(){                                    // 繰り返し実行する関数
    for(int i=0; i<3; i++){
        mogura[i] += random(3) - 1;
        if(mogura[i] < 0 ) mogura[i] = 0;
        if(mogura[i] > 3 ) mogura[i] = 3;
        disp("mogura" + String(i) + String(mogura[i])); // モグラを表示
        for(int j=0; j<33; j++){
            M5.update();                        // ボタン情報を更新
            int btn = M5.BtnA.wasPressed() + 2*M5.BtnB.wasPressed() + 4*M5.BtnC.wasPressed();
            for(int k=0; k<3; k++){
                if((mogura[k] == 3) && ((btn>>k)&1) ){
                    disp("","Hit!");            // 顔を表示
                    // mogura[i] = 4; // イラスト未作成
                    delay(1000);
                }
            }
        }
    }
}

void end(){
    M5.Lcd.drawCentreString("GAME START",256,224,2); // 文字列を表示
    do{                                         // ボタンが押されるまで待機する
        M5.update();                            // ボタン情報を更新
        delay(10);                              // 待ち時間処理
    }while(!M5.BtnC.wasPressed());              // ボタンが押されるまで繰り返す
}
