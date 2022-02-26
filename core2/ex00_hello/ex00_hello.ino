/******************************************************************************
Example 0: Hello, world! for M5Stack CORE2
・LCDに文字を表示します。
・本体上面のボタンを押すと、ボタンに応じたメッセージを表示します。

                                          Copyright (c) 2019-2022 Wataru KUNINO
*******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/core2/arduino

M5Stack Arduino Library API 情報：
https://docs.m5stack.com/en/api/core2/system

【引用コード】
https://github.com/bokunimowakaru/m5s/tree/master/example01_hello
******************************************************************************/

#include <M5Core2.h>                            // M5Stack用ライブラリ組み込み

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.Lcd.begin();                             // M5Stack用Lcdライブラリの起動
    M5.Lcd.fillScreen(BLACK);                   // LCDを消去
    M5.Lcd.setTextSize(2);                      // 文字表示サイズを2倍に設定
    M5.Lcd.println("Example 0 M5Stack LCD");    // LCDにタイトルを表示
}

void loop(){                                    // 繰り返し実行する関数
    M5.update();                                // M5Stack用IO状態の更新
    int btnA = M5.BtnA.wasPressed();            // ボタンAの状態をbtnAへ代入
    int btnB = M5.BtnB.wasPressed();            // ボタンBの状態をbtnBへ代入
    int btnC = M5.BtnC.wasPressed();            // ボタンCの状態をbtnCへ代入

    if( btnA == 1 ){                            // ボタンAが押されていた時
        M5.Lcd.println("Hello, world!");        // LCDへメッセージを表示
    }
    if( btnB == 1 ){                            // ボタンBが押されていた時
        M5.Lcd.println("IoT Device M5Stack");   // LCDへメッセージを表示
    }
    if( btnC == 1 ){                            // ボタンCが押されていた時
        M5.Lcd.fillScreen(BLACK);               // LCDを消去
        M5.Lcd.setCursor(0,0);                  // 文字描画位置を画面左上へ
        M5.Lcd.println("Screen Cleared");       // LCDへメッセージを表示
    }
}
