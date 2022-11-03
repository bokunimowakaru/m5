/*******************************************************************************
Example 9 : ESP32C3 Wi-Fi コンシェルジェ アナウンス担当（音声合成出力）
                                                               for M5Stack Core2

AquosTalkを使った音声合成でユーザへ気づきを通知することが可能なIoT機器です。

    対応IC： AquosTalk Pico LSI

    AquosTalk接続用
    TXD -> AquosTalk Pico LSI側 RXD端子(2番ピン)

    使用機材(例)： M5Stack Core2 + AquosTalk Pico LSI

                                          Copyright (c) 2016-2022 Wataru KUNINO
*******************************************************************************/

#include <M5Core2.h>                        // M5Stack用ライブラリの組み込み
#include <WiFi.h>                           // ESP32用WiFiライブラリ
#include <WebServer.h>                      // HTTPサーバ用ライブラリ

#define PIN_SS2_RX 33                       // シリアル受信ポート(未使用)
#define PIN_SS2_TX 32                       // シリアル送信 AquosTalk Pico LSI側

#define SSID "1234ABCD"                     // 無線LANアクセスポイントのSSID
#define PASS "password"                     // パスワード
#define PORT 1024                           // 受信ポート番号

#define LCD_ORIGIN_Y 2*8+4                      // 表示開始位置
#define LCD_LINES   (27*8 - LCD_ORIGIN_Y)/8     // 表示可能な行数

int lcd_line = 0;                               // 現在の表示位置(行)
char lcd_buf[LCD_LINES][54];                    // LCDスクロール表示用バッファ

void lcd_log(String S){                         // LCDにログを表示する関数
    lcd_line++;                                 // 次の行に繰り上げる
    if(lcd_line >= LCD_LINES){                  // 行末を超えたとき
        lcd_line = LCD_LINES - 1;               // 行末に戻す
        for(int y=1;y<LCD_LINES;y++){           // 1行分、上にスクロールする処理
            int org_y = 8*(y-1)+LCD_ORIGIN_Y;   // 文字を表示する位置を算出
            int sLenX = strlen(lcd_buf[y]) * 6; // 消去する文字のX座標
            M5.Lcd.fillRect(sLenX, org_y, 320-sLenX, 8, DARKGREY); // 文字を消去
            M5.Lcd.setCursor(0, org_y);         // 描画する文字の座標を指定
            M5.Lcd.print(lcd_buf[y]);           // バッファ内のデータを表示する
            memcpy(lcd_buf[y-1],lcd_buf[y],54); // バッファ内データを1行分移動
        }
    }
    int sLenX = S.length() * 6;                 // 消去する文字のX座標
    M5.Lcd.fillRect(sLenX, 8*lcd_line+LCD_ORIGIN_Y, 320 - sLenX, 8, DARKGREY);
    M5.Lcd.setCursor(0, 8*lcd_line+LCD_ORIGIN_Y); // 現在の文字表示位置に移動
    M5.Lcd.print(S.substring(0,53));            // 53文字までをLCDに表示
    S.toCharArray(lcd_buf[lcd_line],54);        // 配列型変換(最大長時\0付与有)
}

HardwareSerial serial2(2);                  // シリアル2を生成
WebServer server(80);                       // Webサーバ(ポート80=HTTP)定義

void handleRoot(){
    char talk[97] = "de-ta'o'nyu-ryo_kushiteku'dasai."; // 音声出力用の文字列
    lcd_log("Connected");                   // 接続されたことを表示
    if(server.hasArg("TEXT")){              // 引数TEXTが含まれていた時
        String rx = server.arg("TEXT");     // 引数TEXTの値を取得し変数rxへ代入
        rx.toCharArray(talk,97);
        trUri2txt(talk);                    // URLエンコードの変換処理
    }
    if(server.hasArg("VAL")){               // 引数VALが含まれていた時
        int i = server.arg("VAL").toInt();  // 引数VALの値を取得し変数rxへ代入
        snprintf(talk,96,"su'-tiwa <NUMK VAL=%d>desu.",i);
    }
    if(strlen(talk) > 0){                   // 文字列が代入されていた場合、
        serial2.print("\r$");               // ブレークコマンドを出力する
        delay(100);                         // 待ち時間処理
        serial2.print(talk);                // 受信文字データを音声出力
        serial2.print('\r');                // 改行コード（CR）を出力する
        lcd_log("> "+String(talk));         // LCDに表示
    }
    String tx = getHtml(talk);              // HTMLコンテンツを取得
    server.send(200, "text/html", tx);      // HTMLコンテンツを送信
}

void setup(){                               // 起動時に一度だけ実行する関数
    M5.begin();                             // M5Stack用ライブラリの起動
    M5.Lcd.setBrightness(31);               // 輝度を下げる（省エネ化）
    M5.Lcd.print("M5Stack eg.9 talk ");     // タイトルをシリアル出力表示
    serial2.begin(9600, SERIAL_8N1, PIN_SS2_RX, PIN_SS2_TX); // シリアル初期化
    serial2.print("\r$");                   // ブレークコマンドを出力する
    delay(100);                             // 待ち時間処理
    serial2.print("$?kon'nnichi/wa.\r");    // 音声「こんにちわ」を出力する
    WiFi.mode(WIFI_STA);                    // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                  // 無線LANアクセスポイントへ接続
    while(WiFi.status() != WL_CONNECTED){   // 接続に成功するまで待つ
        M5.Lcd.print('.');                  // (WS2812)LEDの点滅
        delay(500);                         // 待ち時間処理
    }
    M5.Lcd.println(WiFi.localIP());         // IPアドレスを表示
    serial2.print("<NUM VAL=");             // 数字読み上げ用タグ出力
    serial2.print(WiFi.localIP());          // IPアドレスを読み上げる
    serial2.print(">.\r");                  // タグの終了を出力する
    server.on("/", handleRoot);             // HTTP接続時のコールバック先を設定
    server.begin();                         // Web サーバを起動する
    M5.Lcd.setTextColor(WHITE, BLUE);
    M5.Lcd.drawCentreString("IoT", 55, 28*8, 2);
    M5.Lcd.drawCentreString("Wataru", 160, 28*8, 2);
    M5.Lcd.drawCentreString("IP", 265, 28*8, 2);
    M5.Lcd.setTextColor(WHITE, DARKGREY);
    M5.Lcd.fillRect(0, LCD_ORIGIN_Y, 320, LCD_LINES*8, DARKGREY);
    M5.Lcd.setCursor(0,LCD_ORIGIN_Y);
    M5.Lcd.print("Ready");
}

void loop(){                                // 繰り返し実行する関数
    server.handleClient();                  // クライアントからWebサーバ呼出
    M5.update();                            // ボタン状態の取得
    delay(1);                               // ボタンの誤作動防止
    if(M5.BtnA.wasPressed()){               // ボタンA(左)が押されていた時
        serial2.print("$oshaberiaio-thi-ta'nnma_tsu.\r"); // 音声「IoT端末」
        lcd_log("> oshaberiaio-thi-ta'nnma_tsu.");        // LCDに表示
    }
    if(M5.BtnB.wasPressed()){               // ボタンB(中央)が押されていた時
        serial2.print("$ku'nino/wataru.\r"); // 音声「国野亘」
        lcd_log("> ku'nino/wataru.");        // LCDに表示
    }
    if(M5.BtnC.wasPressed()){               // ボタンC(右)が押されていた時
        serial2.print("<NUM VAL=");         // 数字読み上げ用タグ出力
        serial2.print(WiFi.localIP());      // IPアドレスを読み上げる
        serial2.print(">.\r");              // タグの終了を出力する
        lcd_log("> WiFi.localIP");          // LCDに表示
    }
}

/******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/core2/arduino

M5Stack Arduino Library API 情報：
https://docs.m5stack.com/en/api/core2/system

【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example21_talk
https://github.com/bokunimowakaru/esp/tree/master/2_example/example53_talk
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex09_talk
*******************************************************************************/
