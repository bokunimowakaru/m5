/*******************************************************************************
Example 8 : Wi-Fi コンシェルジェ リモコン担当(赤外線リモコン制御) 
                                                                for M5Stack Core

赤外線リモコンに対応した家電機器を制御します。
LAN内の他の機器から、Webブラウザを使って、遠隔制御することが出来ます。
LAN内の他の機器に、赤外線リモコンの受信信号をブロードキャスト送信します。

    使用機材(例)：M5Stack Core + IR Unit

起動後、M5Stackの右ボタンで、シャープ製テレビの電源をON/OFFするリモコン信号を
送信できるようになります。
左ボタンで、音量を下げます。
通常のリモコンでAEHA形式のリモコン信号を送信し、M5Stackで受信すると、左ボタンに
信号を記憶します。受信するたびに、左ボタンに記憶した信号は更新されます。
中央ボタンでリモコン方式を変更します。AEHA→NEC→SIRCの順序で巡回します。

詳細説明（M5Stackでリモコン送信&受信）
    https://bokunimo.net/blog/esp/2685/

                                          Copyright (c) 2016-2022 Wataru KUNINO
*******************************************************************************/

#include <M5Stack.h>                        // M5Stack用ライブラリの組み込み
#include <WiFi.h>                           // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                        // UDP通信を行うライブラリ
#include <WebServer.h>                      // HTTPサーバ用ライブラリ

#define DATA_LEN_MAX 16                     // リモコンコードのデータ長(byte)
#define DATA_N 4                            // リモコンコードのコード保持数
#define PIN_IR_IN 22                        // IO22 に IR センサを接続(IR Unit)
#define PIN_IR_OUT 21                       // IO21 に IR LEDを接続(IR Unit)

#define SSID "1234ABCD"                     // 無線LANアクセスポイントのSSID
#define PASS "password"                     // パスワード
#define PORT 1024                           // 送信のポート番号
#define DEVICE "ir_rc_1,"                   // デバイス名(5文字+"_"+番号+",")
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

/*
byte DATA[DATA_N][DATA_LEN_MAX] = {         // 保存用・リモコン信号データ
    {0xAA,0x5A,0x8F,0x12,0x16,0xD1},        // AQUOS TV POWER (AEHA len=48)
    {0xAA,0x5A,0x8F,0x12,0x15,0xE1},        // AQUOS TV VOL_DOWN
    {0xAA,0x5A,0x8F,0x12,0x16,0xD1},        // AQUOS TV POWER (AEHA len=48)
    {0xAA,0x5A,0x8F,0x12,0x14,0xF1}         // AQUOS TV VOL_UP
};
int DATA_LEN[DATA_N]={48,48,48,48};         // 保存用・リモコン信号長（bit）
int IR_TYPE[DATA_N]={AEHA,AEHA,AEHA,AEHA};  // 保存用・リモコン方式
int ir_type = AEHA;                         // リモコン方式 255で自動受信
int ir_repeat = 3;                          // 送信リピート回数
*/

/*
byte DATA[DATA_N][DATA_LEN_MAX] = {         // 保存用・リモコン信号データ
    {0xD2,0x6D,0x04,0xFB},                  // Onkyo POWER
    {0xD2,0x6D,0x03,0xFC},                  // Onkyo VOL_DOWN
    {0xD2,0x6D,0x04,0xFB},                  // Onkyo POWER
    {0xD2,0x6D,0x02,0xFD},                  // Onkyo VOL_UP
};
int DATA_LEN[DATA_N]={32,32,32,32};         // 保存用・リモコン信号長（bit）
int IR_TYPE[DATA_N]={NEC,NEC,NEC,NEC};      // 保存用・リモコン方式
int ir_type = NEC;                          // リモコン方式 255で自動受信
int ir_repeat = 3;                          // 送信リピート回数
*/

void disp(int hlight=-1, uint32_t color=0){ // リモコンデータを表示する関数
    char s[97];                             // 文字列変数を定義 97バイト96文字
    const char type_s[][5]={"AEHA","NEC ","SIRC"};  // 各リモコン形式名
    const char btn_s[][7]={"Left","Center","Right"};// 各ボタン名
    M5.Lcd.setCursor(15*6, 0);                // 文字の表示座標を画面上部へ
    M5.Lcd.println(String(type_s[ir_type])+"]"); // 現在のリモコン形式を表示
    M5.Lcd.setCursor(0, 15*8);                // 文字の表示座標を画面下部へ
    M5.Lcd.fillRect(0, 16*8-4, 320, 12*8, DARKGREY);
    for(int i = 0; i < 4; i++){             // 保持データをLCDに表示する処理部
        M5.Lcd.setTextColor(WHITE, (i == hlight)? color : DARKGREY);
        M5.Lcd.print("\n["+String(i)+"] ");
        M5.Lcd.print("IR Type = " + String(type_s[IR_TYPE[i]]));
        M5.Lcd.println(", IR Len. = " + String(DATA_LEN[i]));
        ir_data2txt(s,96,DATA[i],DATA_LEN[i]);
        M5.Lcd.println("    IR Data = " + String(s));
    } // ボタン表示
    M5.Lcd.setTextColor(WHITE, BLUE);
    M5.Lcd.drawCentreString("TX[0] "+String(type_s[IR_TYPE[0]]), 70, 28*8, 2);
    M5.Lcd.drawCentreString("TYPE="+String(type_s[ir_type]), 160, 28*8, 2);
    M5.Lcd.drawCentreString("TX["+String(ir_type+1)+"]", 250, 28*8, 2);
    M5.Lcd.setTextColor(WHITE, BLACK);
}

IPAddress IP_BROAD;                         // ブロードキャストIPアドレス

WiFiUDP udp;                                // UDP通信用のインスタンスを定義
WebServer server(80);                       // Webサーバ(ポート80=HTTP)定義

void handleRoot(){
    char s[97];                             // 文字列変数を定義 97バイト96文字
    M5.Lcd.setCursor(43*6, 0);              // LCD上のカーソル位置を画面上部へ
    M5.Lcd.println("Connected");            // 接続されたことをシリアル出力表示
    if(server.hasArg("TYPE")){              // 引数TYPEが含まれていた時
        IR_TYPE[0] = server.arg("TYPE").toInt(); // 引数TYPEの値をIR_TYPEへ
    }
    if(server.hasArg("IR")){                // 引数IRが含まれていた時
        String rx = server.arg("IR");       // 引数IRの値を取得し変数rxへ代入
        rx.toCharArray(s,97);
        trUri2txt(s);
        DATA_LEN[0]=ir_txt2data(DATA[0],DATA_LEN_MAX,s); // 受信データsをリモコン信号に変換
        disp(0, MAROON);                    // リモコンデータをLCDに表示
        ir_send(DATA[0],DATA_LEN[0],IR_TYPE[0]);
    }
    ir_data2txt(s, 97, DATA[0], DATA_LEN[0]);           // 信号データDを表示文字sに変換
    String tx = getHtml(s,DATA_LEN[0],IR_TYPE[0]);   // HTMLコンテンツを取得
    server.send(200, "text/html", tx);      // HTMLコンテンツを送信
    M5.Lcd.setCursor(43*6, 0);              // LCD上のカーソル位置を画面上部へ
    M5.Lcd.println("         ");            // Connectedを消去
}

void setup(){                               // 起動時に一度だけ実行する関数
    M5.begin();                             // M5Stack用ライブラリの起動
    M5.Lcd.setBrightness(31);               // 輝度を下げる（省エネ化）
    ir_read_init(PIN_IR_IN);                // IRセンサの入力ポートの設定
    ir_send_init(PIN_IR_OUT);               // IR LEDの出力ポートの設定
    M5.Lcd.setTextColor(WHITE,BLACK);       // 文字色を白(背景黒)に設定
    M5.Lcd.print("M5 eg.8 ir_rc [");        // タイトルをLCDに出力
    disp(1, MAROON);                        // タイトルとデータをLCDに出力
    M5.Lcd.setCursor(21*6, 0);              // LCD上のカーソル位置を画面上部へ
    WiFi.mode(WIFI_STA);                    // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                  // 無線LANアクセスポイントへ接続
    while(WiFi.status() != WL_CONNECTED){   // 接続に成功するまで待つ
        M5.Lcd.print(".");                  // 「.」をLCD表示
        delay(500);                         // 待ち時間処理
    }
    server.on("/", handleRoot);             // HTTP接続時のコールバック先を設定
    server.begin();                         // Web サーバを起動する
    M5.Lcd.println(WiFi.localIP());         // 本機のIPアドレスをシリアル表示
    IP_BROAD = WiFi.localIP();              // IPアドレスを取得
    IP_BROAD[3] = 255;                      // ブロードキャストアドレスに
    udp.begin(PORT);                        // UDP通信御開始
}

void loop(){                                // 繰り返し実行する関数
    server.handleClient();                  // クライアントからWebサーバ呼出

    /* 赤外線受信・UDP送信処理 */
    byte d[DATA_LEN_MAX];                   // リモコン信号データ
    char s[97];                             // 文字列変数を定義 97バイト96文字
    int len=ir_read(d,DATA_LEN_MAX,ir_type); // 赤外線信号を読み取る
    if(len >= 16){                          // 16ビット以上の時に以下を実行
        udp.beginPacket(IP_BROAD, PORT);    // UDP送信先を設定
        udp.print(DEVICE);                  // デバイス名を送信
        udp.print(len);                     // 信号長を送信
        udp.print(",");                     // カンマ「,」を送信
        ir_data2txt(s,96,d,len);            // 受信データをテキスト文字に変換
        udp.println(s);                     // それを文字をUDP送信
        udp.endPacket();                    // UDP送信の終了(実際に送信する)
        DATA_LEN[0] = len;                  // データ長lenをD_LENにコピーする
        IR_TYPE[0] = ir_type;               // リモコン方式を保持する
        memcpy(DATA[0],d,DATA_LEN_MAX);     // 受信したリモコン信号をコピー
        disp(0,BLUE);                       // リモコンデータをLCDに表示
        delay(500);                         // 0.5秒の待ち時間処理
    }
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
