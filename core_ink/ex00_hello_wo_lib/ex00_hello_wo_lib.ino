/******************************************************************************
Example 0: Hello, world! for M5Stack 簡易的なink_println版(全画面描画)

・電子ペーパーInk(e-Paper)に文字を表示します。
・本体上面のボタンを押すと、ボタンに応じたメッセージを表示します。
※電源ボタンを押すと電源を切り、操作が出来なくなくなります。
　電子ペーパーInkの画面は消えません（本体の先頭部の緑色LEDが消灯します）
　電源を入れるには、再度、電源ボタンを押してください。

                                          Copyright (c) 2019-2023 Wataru KUNINO
*******************************************************************************
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/coreink/arduino
******************************************************************************/


#include <M5CoreInk.h>                          // M5Stack用ライブラリ組み込み
Ink_Sprite InkPageSprite(&M5.M5Ink);            // M5Inkインスタンス作成
int ink_x = 0;                                  // Ink表示用のX座標
int ink_y = 0;                                  // Ink表示用のY座標

void ink_println(String text){                  // Inkに文字列を表示する
    char c[2];
    for(int i=0; i < text.length(); i++){       // 文字数分の繰り返し処理
        text.substring(i).toCharArray(c, 2);    // 1文字+終端の取り出し
        if(c[0] < 0x20 || c[0] >= 0x7f) continue; // 表示不可文字の処理を排除
        InkPageSprite.drawChar(ink_x,ink_y,c[0]); // バッファに文字を描画
        ink_x += 8;                             // 座標1文字分
        if(ink_x >= 200){                       // X座標が右端を超えた時
            ink_x = 0;                          // X座標を左端へ
            ink_y += 16;                        // Y座標を下の行へ
            if(ink_y >= 200) ink_y = 200 - 16;  // 最下段を超えた時に最下段へ
        }
    }
    InkPageSprite.pushSprite();                 // push the sprite.
    ink_x = 0;                                  // (改行処理)X座標を左端へ
    ink_y += 16;                                // (改行処理)Y座標を下の行へ
    if(ink_y >= 200) ink_y = 200 - 16;          // 最下段を超えた時に最下段へ
}

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    M5.M5Ink.isInit();                          // Inkの初期化
    M5.M5Ink.clear();                           // Inkを消去
    InkPageSprite.creatSprite(0,0,200,200);     // 画像用バッファの作成
    ink_println("Example 0 M5Stack Ink");       // Inkにタイトルを表示
}

void loop(){                                    // 繰り返し実行する関数
    M5.update();                                // M5Stack用IO状態の更新
    int btnA = M5.BtnUP.wasPressed();           // ボタン上の状態をbtnAへ代入
    int btnB = M5.BtnDOWN.wasPressed();         // ボタン下の状態をbtnBへ代入
    int btnM = M5.BtnMID.wasPressed();          // ボタン中央の状態をbtnMへ代入
    int btnP = M5.BtnPWR.wasPressed();          // ボタン側面の状態をbtnPへ代入
    int btnE = M5.BtnEXT.wasPressed();          // ボタン先頭の状態をbtnEへ代入

    if( btnA == 1 ){                            // ボタンAが押されていた時
        ink_println("Hello, world!");           // Inkへメッセージを表示
    }
    if( btnB == 1 ){                            // ボタンBが押されていた時
        ink_println("IoT Device M5Stack");      // Inkへメッセージを表示
    }
    if( btnM == 1 ){                            // ボタン中央が押されていた時
        ink_println("MID Button Pressed");      // Inkへメッセージを表示
    }
    if( btnP == 1 ){                            // ボタン側面が押されていた時
        ink_println("Power Button Pressed");    // Inkへメッセージを表示
        delay(1000);
        ink_println("Power OFF");               // Inkへメッセージを表示
        M5.shutdown();                          // 電源OFF
    }
    if( btnE == 1 ){                            // ボタン先頭が押されていた時
        M5.M5Ink.clear();                       // Inkを消去
        InkPageSprite.clear();
        InkPageSprite.pushSprite();             // push the sprite.
        ink_x = 0;
        ink_y = 0;
        ink_println("Screen Cleared");          // Inkへメッセージを表示
    }
}

/******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/coreink/arduino

M5Stack Arduino Library API 情報：
https://docs.m5stack.com/en/api/coreink/system_api

【引用コード】
https://github.com/bokunimowakaru/m5s/tree/master/example01_hello
******************************************************************************/
