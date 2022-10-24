/*******************************************************************************
Example 8 : Wi-Fi コンシェルジェ リモコン担当(赤外線リモコン制御) 
                                                                for M5Stack Core

赤外線リモコンに対応した家電機器を制御します。 

    使用機材(例)：M5Stack Core + IR Unit

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
    {0xAA,0x5A,0x8F,0x12,0x16,0xD1},        // AQUOS TV POWER (AEHA len=48)
    {0xAA,0x5A,0x8F,0x12,0x15,0xE1},        // AQUOS TV VOL_DOWN
    {0xAA,0x5A,0x8F,0x12,0x16,0xD1},        // AQUOS TV POWER (AEHA len=48)
    {0xAA,0x5A,0x8F,0x12,0x14,0xF1}         // AQUOS TV VOL_UP
};
int DATA_LEN[DATA_N]={48,48,48,48};         // 保存用・リモコン信号長（bit）
int IR_TYPE[DATA_N]={AEHA,AEHA,AEHA,AEHA};  // 保存用・リモコン方式
int ir_type = AEHA;                         // リモコン方式 255で自動受信

void setup(){                               // 起動時に一度だけ実行する関数
    ir_read_init(PIN_IR_IN);                // IRセンサの入力ポートの設定
    ir_send_init(PIN_IR_OUT);               // IR LEDの出力ポートの設定
    M5.begin();                             // M5Stack用ライブラリの起動
    M5.Lcd.setBrightness(31);               // 輝度を下げる（省エネ化）
    M5.Lcd.setTextColor(WHITE);             // 文字色を白(背景なし)に設定
    M5.Lcd.println("M5 eg.8 ir_rc");        // タイトルをLCDに出力
}

void loop(){                                // 繰り返し実行する関数
    byte d[DATA_LEN_MAX];                   // リモコン信号データ
    int d_len;                              // リモコン信号長（bit）
    char s[97];                             // 文字列変数を定義 97バイト96文字

    /* 赤外線受信 */
    d_len=ir_read(d,DATA_LEN_MAX,ir_type);  // 赤外線信号を読み取る
    if(d_len>=16){                          // 16ビット以上の時に以下を実行
        ir_data2txt(s,96,d,d_len);          // 受信データをテキスト文字に変換
        M5.Lcd.print("RX > ");              // 「RX >」をLCDに表示
        M5.Lcd.println(s);                  // 受信データをLCDに表示
        memcpy(DATA[0],d,DATA_LEN_MAX);     // データ変数dを変数Dにコピーする
        DATA_LEN[0] = d_len;                // データ長d_lenをD_LENにコピーする
        IR_TYPE[DATA_N] = ir_read_mode();   // リモコン方式を保持する
        delay(500);
    }

    M5.update();                            // ボタン状態の取得
    delay(1);                               // ボタンの誤作動防止
    int btn;                                // ボタン値を代入する変数
    btn = M5.BtnA.wasReleased()+2*M5.BtnB.wasReleased()+3*M5.BtnC.wasReleased();
    if(btn > 0 && btn < DATA_N){            // ボタン値が1～3のとき
        M5.Lcd.print("TX > ");              // 「TX >」をLCDに表示
        ir_data2txt(s,96,DATA[btn],DATA_LEN[btn]); // 送信データをテキスト文字に
        M5.Lcd.println(s);                  // 送信データをLCDに表示
        ir_send(DATA[btn],DATA_LEN[btn],IR_TYPE[btn]); // 赤外線リモコン送信
    }
    btn=M5.BtnA.pressedFor(999)+2*M5.BtnB.pressedFor(999)+3*M5.BtnC.pressedFor(999);
    if(btn > 0 && btn < DATA_N){            // 長押しボタン値が1～3のとき
        memcpy(DATA[btn],DATA[0],DATA_LEN_MAX); // 受信したリモコン信号をコピー
        DATA_LEN[btn] = DATA_LEN[0];        // 受信したリモコン信号タイプを保存
        IR_TYPE[btn] = IR_TYPE[0];          // 受信したリモコン方式を保存
        M5.Lcd.print("[" + String(btn) + "]> ");   // 保存ボタン番号1～3を表示
        ir_data2txt(s,96,DATA[btn],DATA_LEN[btn]); // 受信データをテキストに変換
        M5.Lcd.println(s);                  // 保存したデータをLCDに表示
        delay(1000);                        // 重複動作防止のために1秒間の待機
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
