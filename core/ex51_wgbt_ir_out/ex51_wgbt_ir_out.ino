/*******************************************************************************
Example 51: [実用] 温湿度センサ＋赤外線リモコン送信機 for M5Stack

                                          Copyright (c) 2022 Wataru KUNINO
********************************************************************************
筆者が実際に使用しているサンプル・プログラムです。
温度と湿度をLCDにレベル・メータ表示、温度指数(疑似)をグラフ表示する機能と
赤外線リモコン送信機能が含まれています。

    使用機材(例)：M5Sack Core + ENV II Unit + 赤外線LED

M5Stack Core の上下どちらかの8ピン端子MISO (IO19)を赤外線LEDに接続してください。

ご注意：
SPIのMISOを当該用途に使用するので、LCDからデータを取得できない場合があります。
LCDへの表示には影響ありません。
（M5.Lcd.readPixelなど、LCDに表示した画像するコマンドが使えないと思います。）

参考文献：
本ファイル末尾に本サンプルを製作するのに使用した元となるプログラムを示します。
*******************************************************************************/

#include <M5Stack.h>                        // M5Stack用ライブラリの組み込み
#include <WiFi.h>                           // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                        // UDP通信を行うライブラリ
#include <Wire.h>                           // 温湿度センサSHT30 I2C通信用
#include <WebServer.h>                      // HTTPサーバ用ライブラリ

#define PIN_IR_OUT 19                       // [重要] 赤外線LEDを接続するIO

#define SSID "1234ABCD"                     // 無線LANアクセスポイントのSSID
#define PASS "password"                     // パスワード
#define ELEVATION 0.0                       // (気圧補正用)測定地点の標高[m]

#define DEVICE  "humid_3,"                  // 温湿度センサのUDP送信デバイス名
#define DEVICE2 "press_3,"                  // 気圧センサのUDP送信デバイス名
#define PORT 1024                           // UDP送信のポート番号
#define NTP_SERVER "ntp.nict.jp"            // NTPサーバのURL
#define NTP_PORT 8888                       // NTP待ち受けポート

#define DATA_LEN_MAX 16                     // リモコンコードのデータ長(byte)
#define DATA_N 4                            // リモコンコードのコード保持数
#define AEHA        0                       // 赤外線送信方式(Panasonic、Sharp)
#define NEC         1                       // 赤外線送信方式 NEC方式
#define SIRC        2                       // 赤外線送信方式 SONY SIRC方式

byte DISP_MODE = 1;                         // 折れ線グラフの表示温度範囲

byte DATA[DATA_N][DATA_LEN_MAX] = {         // 保存用・リモコン信号データ
    {0xAA,0x5A,0x8F,0x12,0x16,0xD1},        // AQUOS TV POWER (AEHA len=48)
    {0x51,0x76,0x20,0x04,0x59},             // LED照明 DOWN (M5Stack ボタンA)
    {0x51,0x76,0x20,0x01,0x5C},             // LED照明 MODE (M5Stack ボタンB)
    {0x51,0x76,0x20,0x02,0x5F},             // LED照明 UP   (M5Stack ボタンC)
};
int DATA_LEN[DATA_N]={48,40,40,40};         // 保存用・リモコン信号長（bit）
int IR_TYPE[DATA_N]={AEHA,NEC,NEC,NEC};     // 保存用・リモコン方式
int ir_repeat[DATA_N] = {3,1,1,1};          // 送信リピート回数
int ir_type = 255;                          // リモコン方式 255で自動受信
int ir_i = 1;                               // リモコン信号の選択番号1～3

IPAddress IP;                               // ブロードキャストIP保存用
float temp, hum, wgbt, temp2, press;        // 温度値、湿度値、WGBT用の変数
WebServer server(80);                       // Webサーバ(ポート80=HTTP)定義
unsigned long TIME = 0;                     // 1970年からmillis()＝0までの秒数

void disp_graph(){
    if(wgbt < 22 && DISP_MODE == 1){
        DISP_MODE = 0;
        lineGraphInit(14, 24);
    }
    if(wgbt > 26 && DISP_MODE == 1){
        DISP_MODE = 2;
        lineGraphInit(24, 34);
    }
    if((wgbt >= 24 && DISP_MODE == 0)||(wgbt <= 24 && DISP_MODE == 2)){
        DISP_MODE = 1;
        lineGraphInit(19, 29);
    }
}

void ir(int i){
    if(i>3 || i<1) return;
    char s[97];                             // 文字列変数を定義 97バイト96文字
    const char type_s[][5]={"AUTO","AEHA","NEC ","SIRC"};  // 各リモコン形式名
    const char btn_s[][7]={"Left","Center","Right"};// 各ボタン名
    Serial.println("Pressed ["+String(btn_s[i-1])+"] Button");
    Serial.print("["+String(i)+"] ");
    Serial.print("IR Type = " + String(type_s[IR_TYPE[i]+1]));
    Serial.println(", Len. = " + String(DATA_LEN[i]));
    ir_data2txt(s,96,DATA[i],DATA_LEN[i]);
    Serial.println("    IR Data = " + String(s));
    Serial.println("     Repeat = " + String(ir_repeat[i]));
    ir_send(DATA[i],DATA_LEN[i],IR_TYPE[i],ir_repeat[i]); // リモコン送信
    delay(500);                             // 0.5秒の待ち時間処理
}

void handleRoot(){
    char s[97];                             // 文字列変数を定義 97バイト96文字
    M5.Lcd.setCursor(43*6, 110 + 92);
    M5.Lcd.println("Connected");            // 接続されたことをシリアル出力表示
    if(server.hasArg("TYPE")){              // 引数TYPEが含まれていた時
        ir_type = server.arg("TYPE").toInt(); // 引数TYPEの値をIR_TYPEへ
        if(ir_type >= 0 && ir_type <= 2) IR_TYPE[0] = ir_type;
    }
    if(server.hasArg("IR")){                // 引数IRが含まれていた時
        String rx = server.arg("IR");       // 引数IRの値を取得し変数rxへ代入
        rx.toCharArray(s,97);
        trUri2txt(s);
        DATA_LEN[0]=ir_txt2data(DATA[0],DATA_LEN_MAX,s); // 受信データsをリモコン信号に変換
        ir_send(DATA[0],DATA_LEN[0],IR_TYPE[0],ir_repeat[0]);
    }
    ir_data2txt(s, 97, DATA[0], DATA_LEN[0]);       // 信号データDを表示文字sに変換
    String tx = getHtml(s,DATA_LEN[0],IR_TYPE[0]);  // HTMLコンテンツを取得
    server.send(200, "text/html", tx);      // HTMLコンテンツを送信
    M5.Lcd.setCursor(43*6, 110 + 92);       // LCD上のカーソル位置を画面上部へ
    M5.Lcd.println("         ");            // Connectedを消去
}

void wifistart(){
    Serial.print("WIFI WIFI_STA");
    delay(100);
    IP = (0,0,0,0);                         // 非接続状態
    Serial.print("...WIFI begin");
    WiFi.begin(SSID,PASS);                  // 無線LANアクセスポイントへ接続
    delay(100);
    Serial.println("...Started");
    delay(100);
    M5.Lcd.fillRect(0, 202, 320, 8, BLACK);
    M5.Lcd.setCursor(0, 110 + 92);
    M5.Lcd.setTextColor(TFT_WHITE);
    int i=0;
    while(WiFi.status() != WL_CONNECTED){   // 接続に成功
        delay(500);                         // 待ち時間処理
        M5.Lcd.print('.');                  // 進捗表示
        Serial.print('.');
        if(i >= 40) return; else i++;
    }
    IP = WiFi.localIP();                    // IPアドレスを取得
    M5.Lcd.println(IP);                     // UDP送信先IPアドレスを表示
    Serial.println(IP);
    Serial.println("Connected");
    IP[3] = 255;                            // ブロードキャストアドレスに

    TIME = getNtpTime(NTP_SERVER,NTP_PORT); // NTPを用いて時刻を取得
    if(TIME) TIME -= millis()/1000;         // 起動後の経過時間を減算
    server.on("/", handleRoot);             // HTTP接続時のコールバック先を設定
    server.begin();                         // Web サーバを起動する
}

float calc_press_h0(float temp, float press){
    temp += 273.15 + 0.0065 * ELEVATION;                    // # 海面(標高)温度
    press /= pow((1.0 - 0.0065 * ELEVATION / temp),5.257);  // # 海面(標高)気圧
    return press;
}

void get_wgbt(){
    temp = i2c_sht30_getTemp() - 0.5;       // 温度値の取得
    hum = i2c_sht30_getHum();               // 湿度値の取得
    temp2=bme280_getTemp() -1.5;            // 温度を取得して変数tempに代入
    press=bme280_getPress();                // 気圧を取得して変数pressに代入
    bme280_print(temp2,hum,press);
    if(temp >= temp2){                      // 温度値が高い方のセンサ値を採用
        wgbt = 0.725 * temp + 0.0368 * hum + 0.00364 * temp * hum - 3.246 + 0.5;
    }else{
        wgbt = 0.725 * temp2+ 0.0368 * hum + 0.00364 * temp2* hum - 3.246 + 0.5;
    }
    analogMeterNeedle(0,temp);              // メータへ表示
    analogMeterNeedle(1,hum);               // メータへ表示
    String S;
    if(TIME){
        char s[20];
        time2txt(s, (millis()/1000) + TIME);
        S = String(wgbt,1)+"C " + String(calc_press_h0(temp,press),0) + "hPa "
          + String(&s[11]);
    }else{
        S = String(wgbt,1)+"C("+String(temp,1)+"C,"+String(hum,0)+"%) "
          + String(calc_press_h0(temp,press),0) + "hPa";
    }
    if(12. < wgbt && wgbt < 30.){
        M5.Lcd.fillRect(0, 210, 320, 30, BLACK);    // 表示部の背景を塗る
    }else{
        M5.Lcd.fillRect(0, 210, 320, 30, TFT_RED);  // 表示部の背景を塗る
    }
    M5.Lcd.drawCentreString(S, 160,210, 4); // 受信文字列を表示
    if(wgbt > 32.){                         // 熱中症への警告
        M5.Speaker.tone(880);               // スピーカ出力 880Hzを出力
        delay(10);                          // 10msの待ち時間処理
        M5.Speaker.end();                   // スピーカ出力を停止する
    }
}


void setup(){                               // 起動時に一度だけ実行する関数
    delay(100);
    M5.begin();
    ir_send_init(PIN_IR_OUT);               // IR LEDの出力ポートの設定
    // M5.Lcd.begin();                      // M5Stack用Lcdライブラリの起動
    delay(100);
    M5.Lcd.setBrightness(31);               // 輝度を下げる（省エネ化）
    delay(100);
    Wire.begin();                           // I2C通信用ライブラリの起動
    analogMeterInit();                      // アナログメータの初期化
    lineGraphInit(19, 29);                  // グラフ初期化(縦軸の範囲指定)
    M5.Lcd.println("My M5Stack Temp, Hum, Press, IR (SHT30 + BMP280)");
    bme280_init();                          // 気圧センサの初期化
//    Serial.begin(115200);
    delay(100);
    WiFi.mode(WIFI_STA);                    // 無線LANをSTAモードに設定
}

void loop(){                                // 繰り返し実行する関数
/*
    wifistart();
*/
    Serial.print("get_wgbt");
    get_wgbt();                             // 温度、湿度、WGBTの取得
    Serial.println("...got");
    disp_graph();
    Serial.print("lineGraphPlot");
    lineGraphPlot(wgbt,0);                  // WGBTをグラフ表示
    Serial.print("...done 0");
    lineGraphPlot(temp,1);                  // tempをグラフ表示
    Serial.print("...done 1");
    Serial.println("...OK!");

    if(WiFi.status() != WL_CONNECTED){
        wifistart();
    }
    if(IP[3]){                              // 接続状態
        WiFiUDP udp;                        // UDP通信用のインスタンス定義
        udp.beginPacket(IP, PORT);          // UDP送信先を設定
        udp.print(DEVICE);                  // デバイス名を送信
        udp.print(temp,1);                  // 温度値を送信
        udp.print(", ");                    // カンマを送信
        udp.println(hum,1);                 // 湿度値を送信
        udp.endPacket();                    // UDP送信の終了(実際に送信する)
        delay(110);                         // 110ミリ秒間の待ち時間処理
    }

    if(IP[3]){                              // 接続状態
        WiFiUDP udp;                        // UDP通信用のインスタンス定義
        udp.beginPacket(IP, PORT);          // UDP送信先を設定
        udp.print(DEVICE2);                 // デバイス名を送信
        udp.print(temp2,1);                 // 温度値を送信
        udp.print(", ");                    // カンマを送信
        udp.println(press,2);               // 気圧値を送信
        udp.endPacket();                    // UDP送信の終了(実際に送信する)
        delay(110);                         // 110ミリ秒間の待ち時間処理
    }

    // 待機処理 //
/*
    delay(110);                             // 110ミリ秒間の待ち時間処理
    Serial.println("WIFI OFF");
    WiFi.disconnect();
    delay(110);                             // 110ミリ秒間の待ち時間処理
    IP = (0,0,0,0);
*/
    int t = millis()/1000;
    int btn_a=0;
    do{
        M5.Lcd.fillRect(307, 194, 13, 8, BLACK);
        M5.Lcd.setCursor(307, 194);
        M5.Lcd.print(60 - (t % 60));

        byte d[DATA_LEN_MAX];               // リモコン信号データ
        while(t == millis()/1000){
            server.handleClient();          // クライアントからWebサーバ呼出
            M5.update();
            delay(1);                       // ボタン誤作動防止
            if(btn_a) btn_a++;
            if(M5.BtnA.wasPressed()){       // ボタンが押されたときに
                Serial.println("Btn A is Pressed");
                btn_a=1;
            }
            if(M5.BtnA.wasReleased()){      // ボタンが離されたとき
                Serial.println("Btn A is Released, "+String(btn_a));
                if(btn_a >= 10 ) ir(1);     // 赤外線の送信を実行する
                btn_a=0;
            }
            if(M5.BtnB.isPressed()) ir(2);
            if(M5.BtnC.isPressed()) ir(3);
        }
        Serial.print(t);
        Serial.print(", ");
        get_wgbt();                         // 温度、湿度、WGBTの取得
        t = millis()/1000;
    }while(t % 60);

    if(t % (3 * 60 * 60) || t == 0){        // 3時間に1回 or オーバフロー時
        unsigned long time = getNtpTime(NTP_SERVER, NTP_PORT); // 時刻を取得
        if(time) TIME = time - millis()/1000;
    }
}

/*******************************************************************************
参考文献
********************************************************************************
Example 5: ESP32 (IoTセンサ) Wi-Fi 温湿度計 SENSIRION製 SHT30/SHT31/SHT35 版
https://github.com/bokunimowakaru/m5/blob/master/core/ex05_hum/ex05_hum.ino

・デジタルI2Cインタフェース搭載センサから取得した温湿度を送信するIoTセンサです。
注意: ENV HATのバージョンによって搭載されているセンサが異なります。
      このプログラムは SHT30 用です。ENV Unit には対応していません。

ENV II Unit  SHT30 + BMP280 + BMM150
********************************************************************************
Example 8 : Wi-Fi コンシェルジェ リモコン担当(赤外線リモコン制御)
https://github.com/bokunimowakaru/m5/tree/master/core/ex08_ir_out

赤外線リモコンに対応した家電機器を制御します。
LAN内の他の機器から、Webブラウザを使って、遠隔制御することが出来ます。
LAN内の他の機器に、赤外線リモコンの受信信号をブロードキャスト送信します。
    使用機材(例)：M5Stack Core + IR Unit

・温湿度センサSHT30から取得した温度値と湿度値を送信するIoTセンサです。
・センサ値は液晶ディスプレイにアナログメータで表示します。
・WGBT(疑似)を計算しグラフに表示します。【追加機能】
・送信頻度を 約1分に1回に抑えました。【追加機能】

********************************************************************************
Example 57: https://github.com/bokunimowakaru/esp/
https://github.com/bokunimowakaru/esp/blob/master/2_example/example57_fs/example57_fs.ino
*******************************************************************************/
