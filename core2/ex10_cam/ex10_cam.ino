/*******************************************************************************
Example 10: Wi-Fi コンシェルジェ カメラ担当 for M5Stack Core2

Webサーバ機能を使って、カメラのシャッターを制御し、撮影した写真を転送します。

    対応カメラ： SeeedStudio Grove Serial Camera Kit

    カメラ接続用： カメラ直付けのGroveケーブルをM5Stackに接続する
    IO21 TX カメラ側はRXD端子(Grove白色,M5用ケーブル黄色)
    IO22 RX カメラ側はTXD端子(Grove黄色,M5用ケーブル白色)

    使用機材(例)：M5Stack Core2 + SeeedStudio Grove Serial Camera Kit

    トラブルシューティング(ピン番号はESP32-WROOM-02用なので読み替えが必要)：
    https://github.com/bokunimowakaru/esp/blob/master/2_example/example20_camG/

                                          Copyright (c) 2016-2022 Wataru KUNINO
*******************************************************************************/

#include <M5Core2.h>                        // M5Stack用ライブラリの組み込み
#include <WiFi.h>                           // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                        // UDP通信を行うライブラリ

#define PIN_SS2_RX 22                       // シリアル受信RX Grove白色
#define PIN_SS2_TX 21                       // シリアル送信TX Grove黄色
#define TIMEOUT 20000                       // タイムアウト 20秒

#define SSID "1234ABCD"                     // 無線LANアクセスポイントのSSID
#define PASS "password"                     // パスワード
#define PORT 1024                           // UDP送信先ポート番号
#define DEVICE "cam_a_5,"                   // デバイス名(カメラ)
#define JPEG_MAX 32768                      // 対応するJPEG最大サイズ(Bytes)

HardwareSerial serial2(2);                  // カメラ接続用シリアルポートESP32C3

WiFiServer server(80);                      // Wi-Fiサーバ(ポート80=HTTP)定義
IPAddress   IP_LOCAL;                       // 本機のIPアドレス
IPAddress   IP_BROAD;                       // ブロードキャストIPアドレス
int size=0;                                 // 画像データの大きさ(バイト)
int update=60;                              // ブラウザのページ更新間隔(秒)
int cam_size=1;                             // 撮影サイズ 0:VGA 1:QVGA 2:QQVGA
int shot = 1;                               // 待機時のJPEGカメラ画像の更新

void sendUdp(String dev, String S){
    WiFiUDP udp;                            // UDP通信用のインスタンスを定義
    udp.beginPacket(IP_BROAD, PORT);        // UDP送信先を設定
    udp.println(dev + S);
    udp.endPacket();                        // UDP送信の終了(実際に送信する)
    M5.Lcd.println("udp://" + IP_BROAD.toString() + ":" + PORT + " " + dev + S);
    delay(200);                             // 送信待ち時間
}

void sendUdp_Fd(uint16_t fd_num){
    sendUdp(DEVICE,String(fd_num)+", http://"+IP_LOCAL.toString()+"/cam.jpg");
}

void lcd_update(int shot = 0){
    M5.Lcd.setCursor(0, 0);                 // 文字表示位置を原点(左上)に
    M5.Lcd.print("M5Stack eg.10 cam ");     // タイトルを表示
    if(WiFi.status() == WL_CONNECTED){
        M5.Lcd.println(WiFi.localIP());
    }else M5.Lcd.println(WiFi.status());
    M5.Lcd.setTextColor(WHITE, BLUE);
    M5.Lcd.drawCentreString(" OneShot ", 65, 28*8, 2);
    M5.Lcd.drawCentreString(" Stop ", 160, 28*8, 2);
    if(shot > 1) M5.Lcd.setTextColor(WHITE, RED);
    M5.Lcd.drawCentreString(" Play ", 255, 28*8, 2);
    M5.Lcd.setTextColor(WHITE, BLACK);
}

void setup(){
    M5.begin();                             // M5Stack用ライブラリの起動
    M5.Lcd.setBrightness(31);               // 輝度を下げる（省エネ化）
    lcd_update();                           // タイトルを表示
    WiFi.mode(WIFI_STA);                    // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                  // 無線LANアクセスポイントへ接続
    serial2.begin(115200, SERIAL_8N1, PIN_SS2_RX, PIN_SS2_TX); // シリアル初期化
    delay(100);                             // 電源の供給待ち
    M5.Lcd.println("Initializing Camera");  // 初期化中の表示
    CamInitialize();                        // カメラの初期化コマンド
    M5.Lcd.println("Setting QVGA");         // 撮影サイズ設定中の表示
    CamSizeCmd(cam_size);                   // 撮影サイズをQVGAに設定
    M5.Lcd.println("Done settings");        // 設定完了表示
    delay(4000);                            // 完了待ち(開始直後の撮影防止対策)
    while(WiFi.status() != WL_CONNECTED){   // 接続に成功するまで待つ
        M5.Lcd.print('.');                  // 接続待ち時間表示
        delay(500);                         // 待ち時間処理
    }
    IP_LOCAL = WiFi.localIP();
    IP_BROAD = IP_LOCAL;
    IP_BROAD[3] = 255;
    server.begin();                         // サーバを起動する
    sendUdp_Fd(0);                          // 起動したことを知らせるUDP送信
}

void loop(){
    WiFiClient client;                      // Wi-Fiクライアントの定義
    uint8_t *buf;                           // JPEGデータ保存用
    buf = (uint8_t *)malloc(JPEG_MAX);      // メモリ確保
    do{
        M5.update();                        // ボタン状態の取得
        delay(1);                           // ボタンの誤作動防止
        if(M5.BtnA.wasPressed()) shot = 1;  // 1枚だけ撮影
        if(M5.BtnB.wasPressed()) shot = 0;  // 撮影の停止
        if(M5.BtnC.wasPressed()) shot = 2;  // 連続撮影
        if(buf && shot){                    // 以下、待機中のカメラ画像表示
            size=CamCapture();              // カメラで写真を撮影する
            if(size <= JPEG_MAX){           // メモリ容量の確認
                jpeg_div_t div = JPEG_DIV_NONE;
                if(cam_size == 0) div = JPEG_DIV_2;
                CamGetData(buf, size);      // JPEGデータをカメラから受信
                M5.Lcd.drawJpg(buf,size,0,0,0,0,0,0,div); // JPEGデータ表示
            }
        }
        lcd_update(shot);                   // タイトルを表示
        if(shot == 1) shot = 0;             // 1枚だけ撮影時に次回の撮影を停止
        client = server.available();        // 接続されたクライアントを生成
    }while(!client);                        // 未接続時は、doループを繰り返す
    free(buf);                              // JPEGデータ用メモリの開放

    char c;                                 // 文字変数を定義
    char s[65];                             // 文字列変数を定義 65バイト64文字
    int len=0, t=0, i, j;                   // HTTPクエリの待ち受け用の変数
    M5.Lcd.println("Connected");            // シリアル出力表示
    while(client.connected()){              // 当該クライアントの接続状態を確認
        if(client.available()){             // クライアントからのデータを確認
            t=0;                            // 待ち時間変数をリセット
            c=client.read();                // データを文字変数cに代入
            if(c=='\n'){                    // 改行を検出した時
                if(len>5 && strncmp(s,"GET /",5)==0) break;
                len=0;                      // 文字列長を0に
            }else if(c!='\r' && c!='\0'){
                s[len]=c;                   // 文字列変数に文字cを追加
                len++;                      // 変数lenに1を加算
                s[len]='\0';                // 文字列を終端
                if(len>=64) len=63;         // 文字列変数の上限
            }
        }
        t++;                                // 変数tの値を1だけ増加させる
        if(t>TIMEOUT) break; else delay(1); // TIMEOUTに到達したらwhileを抜ける
    }
    delay(1);                               // クライアント側の応答待ち時間
    if(!client.connected()||len<6) return;  // 切断された場合はloop()の先頭へ
    M5.Lcd.println(s);                      // 受信した命令を液晶に表示
    if(strncmp(s,"GET / ",6)==0){           // コンテンツ取得命令時
        html(client,size,update,WiFi.localIP()); // コンテンツ表示
        client.flush();                     // ESP32用 ERR_CONNECTION_RESET対策
        client.stop();                      // クライアントの切断
        return;                             // 処理の終了・loop()の先頭へ
    }
    if(strncmp(s,"GET /cam.jpg",12)==0){    // 画像取得指示の場合
        size=CamCapture();                  // カメラで写真を撮影する
        M5.Lcd.print(size);                 // ファイルサイズを表示
        M5.Lcd.println(" Bytes");           // 単位表示
        client.println("HTTP/1.0 200 OK");                  // HTTP OKを応答
        client.println("Content-Type: image/jpeg");         // JPEGコンテンツ
        client.println("Content-Length: " + String(size));  // ファイルサイズ
        client.println("Connection: close");                // 応答後に閉じる
        client.println();                                   // ヘッダの終了
        if(size <= JPEG_MAX){               // メモリ容量の確認
            buf = (uint8_t *)malloc(size);  // メモリ確保
            if(buf != NULL){                // 確保できたかどうかを確認
                jpeg_div_t div = JPEG_DIV_NONE;
                if(cam_size == 0) div = JPEG_DIV_2;
                CamGetData(buf, size);      // JPEGデータをカメラから受信
                M5.Lcd.drawJpg(buf,size,0,0,0,0,0,0,div); // JPEGデータ表示
                client.write((const uint8_t *)buf, size); // JPEGデータ送信
            }else CamGetData(client);       // 描画せずにJPEGデータを送信
            free(buf);                      // JPEGデータ用メモリの開放
        }else CamGetData(client);
        client.println();                   // コンテンツの終了
        client.flush();                     // ESP32用 ERR_CONNECTION_RESET対策
        client.stop();                      // クライアントの切断
        return;                             // 処理の終了・loop()の先頭へ
    }
    if(strncmp(s,"GET /?INT=",10)==0){      // 更新時間の設定命令を受けた時
        update = atoi(&s[10]);              // 受信値を変数updateに代入
    }
    if(strncmp(s,"GET /?SIZE=",11)==0){     // JPEGサイズ設定命令時
        cam_size = atoi(&s[11]);            // 受信値を変数cam_sizeに代入
        CamSizeCmd(cam_size);               // JPEGサイズ設定
    }
    if(!strncmp(s,"GET /favicon.ico",16)){  // Google Chrome対応(追加)
        client.println("HTTP/1.0 404 Not Found");
        client.println();                   // ヘッダの終了
        client.flush();                     // ESP32用 ERR_CONNECTION_RESET対策
        client.stop();                      // クライアントの切断
        return;                             // 処理の終了・loop()の先頭へ
    }

    for(i=6;i<strlen(s);i++) if(s[i]==' '||s[i]=='+') s[i]='\0';
    htmlMesg(client,&s[6],WiFi.localIP());  // メッセージ表示
    client.flush();                         // ESP32用 ERR_CONNECTION_RESET対策
    client.stop();                          // クライアント切断
    M5.Lcd.println("Sent HTML");            // シリアル出力表示
}

/******************************************************************************
【参考文献】
Arduino IDE 開発環境イントール方法：
https://docs.m5stack.com/en/quick_start/core2/arduino

M5Stack Arduino Library API 情報：
https://docs.m5stack.com/en/api/core2/system

HardwareSerial.h：
    void begin(
        unsigned long baud,
        uint32_t config=SERIAL_8N1,
        int8_t rxPin=-1,
        int8_t txPin=-1,
        bool invert=false,
        unsigned long timeout_ms = 20000UL,
        uint8_t rxfifo_full_thrhd = 112
    );

【引用コード】
https://github.com/bokunimowakaru/esp/tree/master/2_example/example20_camG
https://github.com/bokunimowakaru/esp/tree/master/2_example/example52_camG
https://github.com/bokunimowakaru/esp32c3/tree/master/learning/ex10_cam
*******************************************************************************/
