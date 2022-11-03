/*******************************************************************************
Example 8 :赤外線リモコン制御 for M5Stack Core

赤外線リモコンに対応した家電機器を制御します。

    使用機材(例)：M5Stack Core + IR Unit

起動後、M5Stackの右ボタンで、シャープ製テレビの電源をON/OFFするリモコン信号を
送信できるようになります。
左ボタンで、音量を下げます。
通常のリモコンでAEHA形式のリモコン信号を送信し、M5Stackで受信すると、左ボタンに
信号を記憶します。受信するたびに、左ボタンに記憶した信号は更新されます。
中央ボタンでリモコン方式を変更します。AEHA→NEC→SIRCの順序で巡回します。

                                          Copyright (c) 2016-2022 Wataru KUNINO
*******************************************************************************/

#include <M5Stack.h>                        // M5Stack用ライブラリの組み込み

#define DATA_LEN_MAX 16                     // リモコンコードのデータ長(byte)
#define DATA_N 4                            // リモコンコードのコード保持数
#define PIN_IR_IN 22                        // IO22 に IR センサを接続(IR Unit)
#define PIN_IR_OUT 21                       // IO21 に IR LEDを接続(IR Unit)

#define AEHA        0                       // 赤外線送信方式(Panasonic、Sharp)
#define NEC         1                       // 赤外線送信方式 NEC方式
#define SIRC        2                       // 赤外線送信方式 SONY SIRC方式

byte DATA[DATA_N][DATA_LEN_MAX] = {         // 保存用・リモコン信号データ
    {0xAA,0x5A,0x8F,0x12,0x15,0xE1},        // AQUOS TV VOL_DOWN
    {0xAA,0x5A,0x8F,0x12,0x16,0xD1},        // AQUOS TV POWER (AEHA len=48)
    {0xD2,0x6D,0x04,0xFB},                  // Onkyo POWER
    {0x15,0x9A,0x00}                        // SONY POWER
};
int DATA_LEN[DATA_N]={48,48,32,21};         // 保存用・リモコン信号長（bit）
int IR_TYPE[DATA_N]={AEHA,AEHA,NEC,SIRC};   // 保存用・リモコン方式
int ir_type = AEHA;                         // リモコン方式 255で自動受信
int ir_repeat = 3;                          // 送信リピート回数

void disp(int hlight=-1, uint32_t color=0){ // リモコンデータを表示する関数
    char s[97];                             // 文字列変数を定義 97バイト96文字
    const char type_s[][5]={"AEHA","NEC ","SIRC"};  // 各リモコン形式名
    const char btn_s[][7]={"Left","Center","Right"};// 各ボタン名
    M5.Lcd.fillScreen(BLACK);               // 表示内容を消去
    M5.Lcd.setCursor(0, 0);                 // 文字の表示座標を原点(左上)に
    M5.Lcd.print("M5 eg.8 ir_rc [");        // タイトルをLCDに出力
    M5.Lcd.println(String(type_s[ir_type])+"]"); // 現在のリモコン形式を表示
    for(int i = 0; i < 4; i++){             // 保持データをLCDに表示する処理部
        M5.Lcd.println("\n["+String(i)+"] ");
        if(i == hlight) M5.Lcd.setTextColor(WHITE, color);
        M5.Lcd.println("IR Type = " + String(type_s[IR_TYPE[i]]));
        M5.Lcd.println("IR Len. = " + String(DATA_LEN[i]));
        ir_data2txt(s,96,DATA[i],DATA_LEN[i]);
        M5.Lcd.println("IR Data = " + String(s));
        if(i == hlight) M5.Lcd.setTextColor(WHITE, BLACK);
    } // ボタン表示
    M5.Lcd.drawCentreString("TX[0] "+String(type_s[IR_TYPE[0]]), 70, 224, 2);
    M5.Lcd.drawCentreString("TYPE="+String(type_s[ir_type]), 160, 224, 2);
    M5.Lcd.drawCentreString("TX["+String(ir_type+1)+"]", 250, 224, 2);
}

void setup(){                               // 起動時に一度だけ実行する関数
    ir_read_init(PIN_IR_IN);                // IRセンサの入力ポートの設定
    ir_send_init(PIN_IR_OUT);               // IR LEDの出力ポートの設定
    M5.begin();                             // M5Stack用ライブラリの起動
    M5.Lcd.setBrightness(31);               // 輝度を下げる（省エネ化）
    M5.Lcd.setTextColor(WHITE,BLACK);       // 文字色を白(背景黒)に設定
    disp(1, MAROON);                        // タイトルとデータをLCDに出力
}

void loop(){                                // 繰り返し実行する関数

    /* 赤外線受信 */
    byte d[DATA_LEN_MAX];                   // リモコン信号データ
    int len=ir_read(d,DATA_LEN_MAX,ir_type); // 赤外線信号を読み取る
    if(len >= 16){                          // 16ビット以上の時に以下を実行
        DATA_LEN[0] = len;                  // データ長lenをD_LENにコピーする
        IR_TYPE[0] = ir_type;               // リモコン方式を保持する
        memcpy(DATA[0],d,DATA_LEN_MAX);     // 受信したリモコン信号をコピー
        disp(0, BLUE);                      // リモコンデータをLCDに表示
        delay(500);                         // 0.5秒の待ち時間処理
    }

    /* 赤外線送信 */
    M5.update();                            // ボタン状態の取得
    delay(1);                               // ボタンの誤作動防止
    if(M5.BtnA.wasPressed()){               // ボタンA(左)が押されていた時
        disp(0, MAROON);                    // リモコンデータをLCDに表示
        ir_send(DATA[0],DATA_LEN[0],IR_TYPE[0],ir_repeat); // リモコン送信
    }
    if(M5.BtnB.wasPressed()){               // ボタンB(中央)が押されていた時
        ir_type++;                          // リモコン方式を変更
        if(ir_type > 2) ir_type = 0;        // リモコン方式が範囲外のとき0に
        int i = ir_type + 1;                // 変数iにコードのインデックス値を
        disp(i, MAROON);                    // リモコンデータをLCDに表示
    }
    if(M5.BtnC.wasPressed()){               // ボタンC(右)が押されていた時
        int i = ir_type + 1;                // 変数iにコードのインデックス値を
        disp(i, MAROON);                    // リモコンデータをLCDに表示
        ir_send(DATA[i],DATA_LEN[i],IR_TYPE[i],ir_repeat); // リモコン送信
    }
}

/*******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/m5core/arduino

M5Stack Arduino Library API 情報：
https://docs.m5stack.com/en/api/core/system

【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example19_ir_rc
https://github.com/bokunimowakaru/esp/tree/master/2_example/example51_ir_rc
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex08_ir_out

リモコンコード シャープ製TV、パナソニック製Blu-ray、ほか
https://github.com/bokunimowakaru/ir-tester/blob/master/ir_tester_h8tiny/src/lib/ir_data.h
*******************************************************************************/
