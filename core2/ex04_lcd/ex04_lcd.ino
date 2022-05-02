/*******************************************************************************
Example 4: Wi-Fi コンシェルジェ 掲示板担当 for M5Sack Core
・各種IoTセンサが送信したデータをLCDに表示します。
・HTTPによるWebサーバ機能搭載 Wi-FiコンシェルジェがLCDに文字を表示します。

    使用機材(例)：M5Sack Core
    LCD： 320 x 240 ピクセル (53 x 30文字) フォント6x8ピクセル

                                          Copyright (c) 2021-2022 Wataru KUNINO
*******************************************************************************/

#include <M5Core2.h>                            // M5Stack用ライブラリの組み込み
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#include <WebServer.h>                          // HTTPサーバ用ライブラリ

#define SSID "1234ABCD"                         // 無線LANアクセスポイントSSID
#define PASS "password"                         // パスワード
#define PORT 1024                               // 受信ポート番号

WebServer server(80);                           // Webサーバ(ポート80=HTTP)定義

void handleRoot(){
    String rx, tx;                              // 受信用,送信用文字列
    if(server.hasArg("TEXT")){                  // クエリTEXTが含まれていた時
        rx = server.arg("TEXT").substring(0,53); // クエリ値を文字変数rxへ代入
    }
    tx = getHtml(rx);                           // HTMLコンテンツを取得
    server.send(200, "text/html", tx);          // HTMLコンテンツを送信
    M5.Lcd.fillRect(0, 14, 320, 26, NAVY);      // LCDの前回表示位置の消去
    M5.Lcd.drawCentreString(rx, 160, 16, 4);    // 受信した文字列をLCDに表示する
    lcd_log(rx);                                // LCDにログ表示する
}

#define LCD_ORIGIN_Y 80                         // 表示開始位置
#define LCD_LINES   (240 - LCD_ORIGIN_Y)/8      // 表示可能な行数

int lcd_line = 0;                               // 現在の表示位置(行)
char lcd_buf[LCD_LINES][54];                    // LCDスクロール表示用バッファ

void lcd_log(String S){                         // LCDにログを表示する
    lcd_line++;                                 // 次の行に繰り上げる
    if(lcd_line >= LCD_LINES){                  // 行末を超えたとき
        lcd_line = LCD_LINES - 1;               // 行末に戻す
        for(int y=1;y<LCD_LINES;y++){           // 1行分、上にスクロールする処理
            int org_y = 8*(y-1)+LCD_ORIGIN_Y;   // 文字を表示する位置を算出
            M5.Lcd.fillRect(0, org_y, 320, 8, BLACK);
            M5.Lcd.setCursor(0, org_y);
            M5.Lcd.print(lcd_buf[y]);           // バッファ内のデータを表示する
            memcpy(lcd_buf[y-1],lcd_buf[y],54); // バッファ内データを1行分移動
        }
    }
    M5.Lcd.fillRect(0, 8*lcd_line+LCD_ORIGIN_Y, 320, 8, BLACK);
    M5.Lcd.setCursor(0, 8*lcd_line+LCD_ORIGIN_Y); // 現在の文字表示位置に移動
    M5.Lcd.print(S.substring(0,53));            // 53文字までを表示
    S.toCharArray(lcd_buf[lcd_line],54);        // 配列型変換(最大長時\0付与有)
}

String ip2s(uint32_t ip){                       // IPアドレスを文字列に変換
    String S;
    for(int i=0;i<=3;i++){
        S += String((ip>>(8*i))%256);
        if(i<3) S += ".";
    }
    return S;
}

WiFiUDP udp;                                    // UDP通信用のインスタンスを定義

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    M5.Lcd.setBrightness(31);                   // 輝度を下げる（省エネ化）
    M5.Lcd.setTextColor(WHITE);                 // 文字色を白(背景なし)に設定
    M5.Lcd.print("M5 LCD ");                    // 「M5 LCD」をLCDに出力

    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    while(WiFi.status() != WL_CONNECTED){       // 接続に成功するまで待つ
        M5.Lcd.print(".");                      // 「.」をLCD表示
        delay(500);                             // 待ち時間処理
    }
    IPAddress ip = WiFi.localIP();              // 本機のIPアドレスを変数に代入
    M5.Lcd.println(ip);                         // IPアドレスを液晶に表示
    M5.Lcd.fillRect(0, 14, 320, 26, NAVY);      // LCDの前回表示位置の消去
    M5.Lcd.fillRect(0, 46, 320, 26, DARKCYAN);  // LCDの前回表示位置の消去
    M5.Lcd.drawCentreString("http://"+ip2s(ip)+"/", 160, 16, 4);
    M5.Lcd.drawCentreString("udp:"+ip2s(ip)+":"+String(PORT), 160, 48, 4);
    server.on("/", handleRoot);                 // HTTP接続時コールバック先設定
    server.begin();                             // Web サーバを起動する
    udp.begin(PORT);                            // UDP通信御開始
    M5.Lcd.drawString("Log:", 0, LCD_ORIGIN_Y); // ログ表示
}

void loop(){                                    // 繰り返し実行する関数
    server.handleClient();                      // クライアントからWebサーバ呼出
    char lcd[54];                               // 表示用変数(49バイト48文字)
    memset(lcd, 0, 54);                         // 文字列変数lcd初期化(49バイト)
    int len = udp.parsePacket();                // 受信パケット長を変数lenに代入
    if(len==0)return;                           // 未受信のときはloop()の先頭に
    udp.read(lcd, 53);                          // 受信データを文字列変数lcdへ
    udp.flush();                                // 受信できなかったデータを破棄
    M5.Lcd.fillRect(0, 46, 320, 26, DARKCYAN);  // LCDの前回表示位置の消去
    M5.Lcd.drawCentreString(lcd, 160, 48, 4);   // 受信した文字列をLCDに表示する
    lcd_log(lcd);                               // LCDにログ表示する
}

/*******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/m5core/arduino

M5Stack Arduino Library API 情報：
https://docs.m5stack.com/en/api/core/system

【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example05_lcd
https://github.com/bokunimowakaru/esp/tree/master/2_example/example18_lcd
https://github.com/bokunimowakaru/esp/tree/master/2_example/example37_lcd
https://github.com/bokunimowakaru/esp/tree/master/2_example/example50_lcd
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex04_lcd
*******************************************************************************/
