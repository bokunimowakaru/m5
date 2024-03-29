/******************************************************************************
Example 0: Hello, world! for M5Stack

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

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    ink_print_setup();                          // Inkの初期化(ink_print.ino)
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
        ink_print_clear();                      // Inkを消去
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
