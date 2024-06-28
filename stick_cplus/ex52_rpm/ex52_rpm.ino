/******************************************************************************
Example 52: 加速度センサMPU6886で回転数を測定し、Wi-Fiで送信する

    使用機材(例)：M5StickC Plus (加速度センサは本体内蔵品)
    
    使用方法・詳細：
    https://bokunimo.net/blog/audio/4592/

    画面サイズ：240 x 135
    
                                               Copyright (c) 2024 Wataru KUNINO
*******************************************************************************
本ソフトウェアには下記のソースコードの一部(MPU6886に関する)が含まれています。
This code was forked by Wataru KUNINO from the following authors:
*******************************************************************************
* Copyright (c) 2021 by M5Stack
*                  Equipped with M5StickC-Plus sample source code
*                          配套  M5StickC-Plus 示例源代码
* Visit for more information: https://docs.m5stack.com/en/core/m5stickc_plus
* 获取更多资料请访问: https://docs.m5stack.com/zh_CN/core/m5stickc_plus
*
* Describe: MPU6886.
* Date: 2021/9/14
*******************************************************************************
*/

#include <M5StickCPlus.h>
#include <WiFi.h>               // ESP32用WiFiライブラリ
#include <WebServer.h>          // HTTPサーバ用ライブラリ
#include <WiFiUdp.h>            // UDP通信を行うライブラリ

#define SSID "1234ABCD"         // 無線LANアクセスポイントSSID
#define PASS "password"         // パスワード
#define PORT 1024               // 送信のポート番号
#define DEVICE "accem_5,"       // デバイス名(5字+"_"+番号+",")
#define UDP_TX_MS   5000        // UDP送信間隔(ms)
#define RADIUS_CM   13          // プラッタの中心からM5Stickまでの距離(cm)
#define AVR_N       30          // 平均回数
#define INTERVAL_MS 200         // 測定間隔(ms)
#define GRAV_MPS2   9.80665     // 重力加速度(m/s2)
#define DEG_MAX     6.0         // 水平レベル測定用最大角度
#define ERR_MAX     20.         // RPM測定用最大誤差(%)
#define RPM_TYP_33  33.333      // 33 1/3 回転RPM測定用基準値(RPM)
#define RPM_TYP_45  45.000      // 45 回転時 RPM測定用基準値(RPM)
#define BUF_N       10          // 残像表示用バッファ数
#define CSV_N       212         // CSV用バッファ数(212固定)

RTC_DATA_ATTR float CAL_GyroX = 0.; // 角速度Xキャリブレーション値
RTC_DATA_ATTR float CAL_GyroY = 0.; // 角速度Yキャリブレーション値
RTC_DATA_ATTR float CAL_GyroZ = 0.; // 角速度Zキャリブレーション値
RTC_DATA_ATTR float CAL_AccmX = 0.; // 加速度Xキャリブレーション値(加算値)
RTC_DATA_ATTR float CAL_AccmY = 0.; // 加速度Yキャリブレーション値(加算値)
RTC_DATA_ATTR float CAL_AccmZ = 1.; // 加速度Zキャリブレーション値(乗算値)

WiFiUDP udp;                    // UDP通信用のインスタンスを定義
IPAddress UDPTO_IP = {255,255,255,255}; // UDP宛先 IPアドレス

WebServer server(80);           // Webサーバ(ポート80=HTTP)定義

int pos_x_prev[BUF_N];          // = 120;
int pos_y_prev[BUF_N];          // = 67;
int rpm1_y_prev[BUF_N];         // = 67;
int rpm2_y_prev[BUF_N];         // = 67;
int udp_len_prev = 1;
unsigned long started_time_ms = millis();
volatile int disp_mode = 0;     // 表示モード(ボタンで切り替わる)
volatile boolean disp_pause = false; // 表示の一時停止
int i_rpm = CSV_N - 1;          // RPM測定結果の保存位置
uint16_t rpm[CSV_N];            // RPM測定結果 1000倍値
uint16_t wow[CSV_N];            // WOW測定結果 100倍値
uint16_t level[CSV_N];          // 角度測定結果 1000倍値
uint16_t meas[CSV_N];           // 測定時刻
int line_x = 27;                // 折れ線グラフのX座標
float rpm_typ = RPM_TYP_33;     // 33回転を設定

void buf_init(){
    for(int i=0; i<BUF_N; i++){
        pos_x_prev[i] = 120; pos_y_prev[i]  = 67;
        rpm1_y_prev[i] = 67; rpm2_y_prev[i] = 67;
    }
}

void buf_append(int *array, int val){
    for(int i=1; i<BUF_N; i++){
        array[i-1] = array[i];
    }
    array[BUF_N-1] = val;
}

void lcd_init(int mode){
    char s[65];
    uint32_t ip = WiFi.localIP();
    switch(mode){
      case 0: // 通常モード、水平計＋回転数計
        M5.Lcd.setRotation(1);     // Rotate the screen. 将屏幕旋转
        M5.Lcd.setTextFont(1);     // 8x6ピクセルのフォント
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 0);    // set the cursor location.  设置光标位置
        // M5StickC Plus LCD 240 x 135 (40 x 17) フォント8x6ピクセル
        M5.Lcd.drawCircle(120, 67, 66, DARKGREY);
    //  M5.Lcd.drawCircle(120, 67, 67, WHITE);
        M5.Lcd.fillCircle(120, 67, 1, DARKGREY);
        M5.Lcd.drawRect(191, 1, 18, 133, DARKGREY);
    //  M5.Lcd.drawRect(190, 0, 20, 135, WHITE);
        M5.Lcd.drawRect(216, 1, 18, 133, DARKGREY);
    //  M5.Lcd.drawRect(215, 0, 20, 135, WHITE);
      break;
      case 1: // 回転数のグラフ表示
        M5.Lcd.setRotation(1);     // Rotate the screen. 将屏幕旋转
        M5.Lcd.setTextFont(1);     // 75ピクセルのフォント(数値表示)
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0, 0);    // set the cursor location.
        M5.Lcd.drawRect(26, 1, 213, 133, DARKGREY);
        M5.Lcd.setCursor(1,1); M5.Lcd.printf("%4.1f",rpm_typ*(1+ERR_MAX/100.));
        M5.Lcd.drawLine(26, 34, 238, 34, DARKGREY);
        M5.Lcd.setCursor(1,28); M5.Lcd.printf("%4.1f",rpm_typ*(1+ERR_MAX/200.));
        M5.Lcd.drawLine(26, 67, 238, 67, DARKGREY);
        M5.Lcd.setCursor(1,61); M5.Lcd.printf("%4.1f",rpm_typ);
        M5.Lcd.drawLine(26, 100, 238, 100, DARKGREY);
        M5.Lcd.setCursor(1,97); M5.Lcd.printf("%4.1f",rpm_typ*(1-ERR_MAX/200.));
        M5.Lcd.setCursor(1,129); M5.Lcd.printf("%4.1f",rpm_typ*(1-ERR_MAX/100.));
        line_x = 27;
      break;
      case 2: // 回転数の数値表示
        M5.Lcd.setRotation(1);     // Rotate the screen. 将屏幕旋转
        M5.Lcd.setTextFont(8);     // 75ピクセルのフォント(数値表示)
        M5.Lcd.fillScreen(BLACK);
      break;
      case  3: // QRコード表示
        M5.Lcd.setRotation(1);     // Rotate the screen. 将屏幕旋转
        M5.Lcd.setTextFont(1);     // 75ピクセルのフォント(数値表示)
        M5.Lcd.fillScreen(BLACK);
        snprintf(s,65,"http://%d.%d.%d.%d/",ip&255,ip>>8&255,ip>>16&255,ip>>24);
        M5.Lcd.setCursor(1,1); M5.Lcd.print(s);
        M5.Lcd.qrcode(s, 114, 10, 123, 2);
      break;
      case -1:
      break;
      default:
        M5.Lcd.setRotation(1);     // Rotate the screen. 将屏幕旋转
        M5.Lcd.setTextFont(1);     // 8x6ピクセルのフォント
        M5.Lcd.fillScreen(BLACK);
    }
}

void lcd_val(float val){
    val += 0.005;
    M5.Lcd.setTextFont(8);
    M5.Lcd.setCursor(0, 44);
    M5.Lcd.printf("%2d ", int(val));
    M5.Lcd.setCursor(128, 44);
    M5.Lcd.printf("%02d", int(val*100)%100);
    M5.Lcd.fillRect(116, 112, 8, 8, WHITE);
    M5.Lcd.setTextFont(1);
}

void handleRoot(){
    String rx, tx;                              // 受信用,送信用文字列
    if(server.hasArg("mode")){                  // 引数Lが含まれていた時
        rx = server.arg("mode");                // 引数Lの値を変数rxへ代入
        if(rx.toInt() < 0){
            disp_pause = true;
        }else{
            disp_mode = rx.toInt();             // 変数sの数値をled_statへ
            lcd_init(disp_mode);
            disp_pause = false;
        }
    }
    if(server.hasArg("stop")){                  // 引数Lが含まれていた時
        rx = server.arg("stop");                // 引数Lの値を変数rxへ代入
        disp_pause = !disp_pause;
    }
    int wow_prev = i_rpm > 0 ? wow[i_rpm - 1] : wow[CSV_N-1];
    wow_prev -= wow[i_rpm];
    int wow_disp = (-100 < wow_prev && wow_prev < 100) ? wow[i_rpm] : 10000;
    tx = getHtml(disp_mode, 
        (float)level[i_rpm]/1000.,
        (float)rpm[i_rpm]/1000., 
        (float)wow_disp/100.,
        disp_pause
    );
    server.send(200, "text/html", tx);          // HTMLコンテンツを送信
}

void handleCSV(){
    String tx = "time(ms), level(deg), rpm(rpm), wow(%)\n";
    int i = i_rpm;
    uint16_t ms = i < CSV_N-1 ? meas[i+1] : meas[0];
    for(int t=0; t < CSV_N; t++){
        i++;
        if(i >= CSV_N) i = 0;
        uint16_t ms_ui = meas[i] - ms;
        tx += String((int)ms_ui)+", "
            + String((float)level[i]/1000.,3)+", "
            + String((float)rpm[i]/1000.,3)+", "
            + String((float)wow[i]/100.,2)+"\r\n";
    }
    server.send(200, "text/csv", tx);           // HTMLコンテンツを送信
}

void handleBMP(){
    uint8_t buf[240*3];
    WiFiClient client = server.client();
    client.println("HTTP/1.0 200 OK");                  // HTTP OKを応答
    client.println("Content-Type: image/bmp");          // コンテンツ
    client.println("Content-Length: " + String(240*135*3+54));  // サイズ
    client.println("Connection: close");                // 応答後に閉じる
    client.println();                                   // ヘッダの終了
    getBmpHeader(buf);
    client.write((const uint8_t *)buf, 54); 
    for(int y=0; y<135; y++){
        getBmpLine(buf,y);
        client.write((const uint8_t *)buf, 240*3);
    }
    client.println();                   // コンテンツの終了
    client.flush();                     // ESP32用 ERR_CONNECTION_RESET対策
    client.stop();                      // クライアントの切断
}

/* After M5StickC Plus is started or reset
  the program in the setUp () function will be run, and this part will only be
  run once. 在 M5StickC Plus
  启动或者复位后，即会开始执行setup()函数中的程序，该部分只会执行一次。 */
void setup() {
    M5.begin();                // Init M5StickC Plus.  初始化 M5StickC Plus
    M5.Imu.Init();             // Init IMU.  初始化IMU
    buf_init();
    lcd_init(0);
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    server.on("/", handleRoot);                 // HTTP接続用コールバック先設定
    server.on("/rpm.csv", handleCSV);           // CSVデータ取得用
    server.on("/screen.bmp", handleBMP);        // 画像データ取得用
    server.begin();                             // Web サーバを起動する
    for(int i=0; i<CSV_N; i++){                 // CSV出力用データの初期化
        rpm[i]=0; wow[i]=0; level[i]=0; meas[i]=0;
    }
}

/* After the program in setup() runs, it runs the program in loop()
The loop() function is an infinite loop in which the program runs repeatedly
在setup()函数中的程序执行完后，会接着执行loop()函数中的程序
loop()函数是一个死循环，其中的程序会不断的重复运行 */

void loop() {
    server.handleClient();
    M5.update();                                // ボタン状態の取得
    if(M5.BtnA.wasPressed()){                   // (過去に)ボタンが押された時
        disp_mode++;
        if(disp_mode > 3) disp_mode = 0;
        disp_pause = false;
        lcd_init(disp_mode);
    }
    if(disp_pause || disp_mode == 3) return;

    float gyroX = 0.; float gyroY = 0.; float gyroZ = 0.;
    float accX = 0.;  float accY = 0.;  float accZ = 0.;
    float temp = 0.;
    float gx, gy, gz, ax, ay, az, t;    
    float rpm1, rpm2;

    for( int i=0; i<AVR_N; i++){
        M5.Imu.getGyroData(&gx, &gy, &gz);
        M5.Imu.getAccelData(&ax, &ay, &az);
        M5.Imu.getTempData(&t);
        gyroX -= gy; gyroY -= gx; gyroZ -= gz; 
        accX  -= ay; accY  -= ax; accZ  -= az; 
        temp  += t;
        delay(INTERVAL_MS / AVR_N);
    }
    gyroX /= AVR_N; gyroY /= AVR_N; gyroZ /= AVR_N;
    accX *= GRAV_MPS2/AVR_N; accY *= GRAV_MPS2/AVR_N; accZ *= GRAV_MPS2/AVR_N;
    temp  /= AVR_N;
    
    // 簡易キャリブレーション部 (側面ボタン押下時)
    if(disp_mode == 0 && M5.BtnB.wasPressed()){
        M5.Lcd.setCursor(0, 4);
        M5.Lcd.println("[Calb]");
        delay(1000);
        CAL_GyroX = -gyroX;
        CAL_GyroY = -gyroY;
        CAL_GyroZ = -gyroZ;
        CAL_AccmX = -accX;
        CAL_AccmY = -accY;
        CAL_AccmZ = -GRAV_MPS2 / accZ;
        M5.Lcd.printf(" %5.1f\n %5.1f\n %5.1f\n", gyroX, gyroY, gyroZ);
        delay(1000);
    }
    gyroX += CAL_GyroX; gyroY += CAL_GyroY; gyroZ += CAL_GyroZ;
    accX += CAL_AccmX; accY += CAL_AccmY; accZ *= CAL_AccmZ;
    if(accZ == 0.) accZ = 1e-38;
    if(gyroZ == 0.) gyroZ = 1e-38;
    // 側面を下向きに置いた場合の簡易対応
    if(accY < -4.9){ // -0.5 * GRAV_MPS2
        float tmp;
        tmp = accZ;  accZ  = accY;  accY  = -tmp;
        tmp = gyroZ; gyroZ = gyroY; gyroY = -tmp;
    }
    
    // 水平計、回転数計、演算処理部
    float degx = atan(accX/accZ) / 6.2832 * 360.;
    float degy = atan(accY/accZ) / 6.2832 * 360.;
    float deg = sqrt(pow(degx,2)+pow(degy,2));
    rpm1 = fabs(gyroZ) / 6.;
    rpm2 = 60 * sqrt(fabs(accY) / 39.478 / RADIUS_CM * 100);
    rpm_typ = rpm1 < 38.73 ? RPM_TYP_33 : RPM_TYP_45;
    int rpm1_y = int(67.5 - 67. * (rpm1 - rpm_typ) / rpm_typ / ERR_MAX * 100);
    int rpm2_y = int(67.5 - 67. * (rpm2 - rpm_typ) / rpm_typ / ERR_MAX * 100);
    int flag_dx = 0;
    
    // モードに応じた処理部（メインボタン押下で切り替え）
    switch(disp_mode){
      case 0:  // 通常モード、水平計＋回転数計
        /**** Gryo ****/
        M5.Lcd.setCursor(0, 4);
        M5.Lcd.println("[Gyro]");
        M5.Lcd.printf("%6.1f\n%6.1f\n%6.1f\n", gyroX, gyroY, gyroZ);
        
        /**** Accel ****/
        M5.Lcd.println("[Accel]");
        M5.Lcd.printf("%6.2f\n%6.2f\n%6.2f\n", accX, accY, accZ);
        
        /**** Temperature ****/
    //  M5.Lcd.printf("[Temperature]\n  %6.2f C\n", temp);

        /**** LEVEL ****/
        M5.Lcd.println("[Level]");
        M5.Lcd.printf("%6.2f\n%6.2f\n", degx, degy);
        M5.Lcd.fillCircle(pos_x_prev[0], pos_y_prev[0], 2, BLACK);
        M5.Lcd.fillCircle(120, 67, 1, DARKGREY);
        for(int i=1; i<BUF_N; i++){
            M5.Lcd.fillCircle(pos_x_prev[i], pos_y_prev[i], 2, DARKGREEN);
        }
        if(deg < DEG_MAX){
            int pos_x = -int(64 * degx / DEG_MAX + 0.5) + 120;
            int pos_y = -int(64 * degy / DEG_MAX + 0.5) +  67;
            M5.Lcd.fillCircle(pos_x, pos_y, 2, RED);
            buf_append(pos_x_prev, pos_x);
            buf_append(pos_y_prev, pos_y);
        }
        
        /**** RPM ****/
        M5.Lcd.println("[RPM]");
        M5.Lcd.printf("%6.2f\n%6.2f\n", rpm1, rpm2);
        M5.Lcd.drawLine(192, rpm1_y_prev[0], 207, rpm1_y_prev[0], BLACK);
        M5.Lcd.drawLine(217, rpm2_y_prev[0], 232, rpm2_y_prev[0], BLACK);
        M5.Lcd.drawLine(192, 67, 207, 67, DARKGREY);
        M5.Lcd.drawLine(217, 67, 232, 67, DARKGREY);
        for(int i=1; i<BUF_N; i++){
            M5.Lcd.drawLine(192, rpm1_y_prev[i], 207, rpm1_y_prev[i], DARKGREEN);
            M5.Lcd.drawLine(217, rpm2_y_prev[i], 232, rpm2_y_prev[i], DARKGREEN);
        }
        if(rpm1_y >= 2 && rpm1_y <= 132){
            M5.Lcd.drawLine(192, rpm1_y, 207, rpm1_y, RED);
            buf_append(rpm1_y_prev, rpm1_y);
        }
        if(rpm2_y >= 2 && rpm2_y <= 132){
            M5.Lcd.drawLine(217, rpm2_y, 232, rpm2_y, RED);
            buf_append(rpm2_y_prev, rpm2_y);
        }
        break;
      case 1:  // 回転数のグラフ表示
        M5.Lcd.setCursor(0, 31);
        if(line_x > 238) break;
        if(rpm1_y >= 2 && rpm1_y <= 132){
            if(line_x > 27) M5.Lcd.drawLine(line_x-1, rpm1_y_prev[BUF_N-1], line_x, rpm1_y, RED);
            buf_append(rpm1_y_prev, rpm1_y);
            flag_dx = 1;
        }
        if(rpm2_y >= 2 && rpm2_y <= 132){
            if(line_x > 27) M5.Lcd.drawLine(line_x-1, rpm2_y_prev[BUF_N-1], line_x, rpm2_y, BLUE);
            buf_append(rpm2_y_prev, rpm2_y);
            flag_dx = 1;
        }
        if(flag_dx) line_x++;
        break;
      case 2:  // 回転数の数値表示
        /*
        M5.Lcd.setCursor(24, 31);
        M5.Lcd.printf("%4.1f",rpm1);
        */
        lcd_val(rpm1);
        break;
    }
    
    // WOW推定値の計算
    i_rpm++;
    if(i_rpm > CSV_N-1) i_rpm = 0;
    meas[i_rpm] = (uint16_t) millis();
    rpm[i_rpm] = int(rpm1 * 1000. + 0.5);   // RPM値の保存
    float mse = 0., avr = 0.;
    for(int i=0; i<CSV_N; i++) avr += (float)rpm[i]/1000.; 
    avr /= CSV_N;
    for(int i=0; i<CSV_N; i++){
        mse += pow(avr-(float)rpm[i]/1000.,2);
    }
    wow[i_rpm] = int(AVR_N * sqrt(mse) / CSV_N / avr * 10000. +.5);
    level[i_rpm] = int(deg * 1000. +.5);
    
    // CSVxUDP送信
    if(rpm1 > 10. && millis()-started_time_ms > UDP_TX_MS && WiFi.status() == WL_CONNECTED){
        started_time_ms = millis();
        String S = String(DEVICE)
            + String(accX,1) + "," + String(accY,1) + "," + String(accZ,1)
            + "," + String(rpm1,2)+ "," + String(rpm2,2);
        if(S.length() > 9){
            udp_len_prev = S.length() - 8;
            udp.beginPacket(UDPTO_IP,PORT); // UDP送信先を設定
            udp.println(S);                 // 送信データSをUDP送信
            udp.endPacket();                // UDP送信の終了(実際に送信する)
        }
        if(disp_mode == 0){
            M5.Lcd.println("[CSVxUDP]");
            M5.Lcd.fillRect(6, 126, 6*udp_len_prev, 8, BLACK);
            M5.Lcd.setCursor(6, 126);
            M5.Lcd.print(S.substring(8));   // 送信データSを表示
        }
    }
}

/******************************************************************************
CSVxUDP 受信結果の一例
UDPデータは、「加速度X,Y,Z,回転数」の形式で送信される
*******************************************************************************
YYYY/MM/dd hh:mm, IP Address, Accelerometer X(m/s2), Accelerometer Y(m/s2), Accelerometer Z(m/s2)
2024/06/08 15:26, 192.168.1.4, -0.1, -1.3, 10.6, 34.29
2024/06/08 15:26, 192.168.1.4, -0.1, -1.3, 10.6, 34.23
2024/06/08 15:26, 192.168.1.4, -0.2, -1.4, 10.6, 34.2
2024/06/08 15:27, 192.168.1.4, -0.2, -1.4, 10.5, 34.19
2024/06/08 15:27, 192.168.1.4, -0.5, -1.4, 9.8, 1.1
2024/06/08 15:27, 192.168.1.4, 0, -0.3, 10.5, 34.26
2024/06/08 15:27, 192.168.1.4, 0, -0.3, 10.6, 34.29
2024/06/08 15:27, 192.168.1.4, 0, -0.2, 10.5, 34.24
2024/06/08 15:27, 192.168.1.4, 0, -0.2, 10.5, 34.19
2024/06/08 15:27, 192.168.1.4, -0.1, -0.2, 10.5, 34.23
2024/06/08 15:27, 192.168.1.4, -0.1, -0.3, 10.6, 34.21
2024/06/08 15:27, 192.168.1.4, -0.1, -0.3, 10.5, 34.26
2024/06/08 15:27, 192.168.1.4, -0.2, -0.4, 10.5, 34.24
2024/06/08 15:27, 192.168.1.4, -0.1, -0.4, 10.6, 34.19
2024/06/08 15:28, 192.168.1.4, -0.1, -0.4, 10.5, 34.13
2024/06/08 15:28, 192.168.1.4, 0, -0.4, 10.6, 34.17
2024/06/08 15:28, 192.168.1.4, 0, -0.4, 10.5, 34.18
2024/06/08 15:28, 192.168.1.4, 0.1, -0.4, 10.5, 34.21
2024/06/08 15:28, 192.168.1.4, 0, -0.3, 10.6, 34.2
2024/06/08 15:28, 192.168.1.4, 0, -0.3, 10.5, 34.14
2024/06/08 15:28, 192.168.1.4, 0, -0.2, 10.6, 34.13
2024/06/08 15:28, 192.168.1.4, -0.1, -0.2, 10.6, 34.16
2024/06/08 15:28, 192.168.1.4, -0.1, -0.2, 10.5, 34.21
2024/06/08 15:28, 192.168.1.4, -0.2, -0.3, 10.5, 34.14
2024/06/08 15:28, 192.168.1.4, -0.1, -0.4, 10.6, 34.08
******************************************************************************/
