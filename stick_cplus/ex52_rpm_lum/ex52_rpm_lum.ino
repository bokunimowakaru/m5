/******************************************************************************
Example 52: 加速度センサMPU6886で回転数を測定し、Wi-Fiで送信する
・

    使用機材(例)：M5StickC Plus + HAT-DLIGHT

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
#define RPM_TYP     33.333      // RPM測定用基準値(RPM)
#define BUF_N       10          // 残像表示用バッファ数

RTC_DATA_ATTR float CAL_GyroX = 0.; // 角速度Xキャリブレーション値
RTC_DATA_ATTR float CAL_GyroY = 0.; // 角速度Yキャリブレーション値
RTC_DATA_ATTR float CAL_GyroZ = 0.; // 角速度Zキャリブレーション値
RTC_DATA_ATTR float CAL_AccmX = 0.; // 加速度Xキャリブレーション値(加算値)
RTC_DATA_ATTR float CAL_AccmY = 0.; // 加速度Yキャリブレーション値(加算値)
RTC_DATA_ATTR float CAL_AccmZ = 1.; // 加速度Zキャリブレーション値(乗算値)

WiFiUDP udp;                    // UDP通信用のインスタンスを定義
IPAddress UDPTO_IP = {255,255,255,255}; // UDP宛先 IPアドレス

int pos_x_prev[BUF_N];          // = 120;
int pos_y_prev[BUF_N];          // = 67;
int rpm1_y_prev[BUF_N];         // = 67;
int rpm2_y_prev[BUF_N];         // = 67;
int udp_len_prev = 1;
unsigned long started_time_ms = millis();
int disp_mode = 0;              // 表示モード(ボタンで切り替わる)
int line_x = 27;                // 折れ線グラフのX座標

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
        M5.Lcd.setCursor(1,1); M5.Lcd.printf("%4.1f",RPM_TYP*(1+ERR_MAX/100.));
        M5.Lcd.drawLine(26, 34, 238, 34, DARKGREY);
        M5.Lcd.setCursor(1,28); M5.Lcd.printf("%4.1f",RPM_TYP*(1+ERR_MAX/200.));
        M5.Lcd.drawLine(26, 67, 238, 67, DARKGREY);
        M5.Lcd.setCursor(1,61); M5.Lcd.printf("%4.1f",RPM_TYP);
        M5.Lcd.drawLine(26, 100, 238, 100, DARKGREY);
        M5.Lcd.setCursor(1,97); M5.Lcd.printf("%4.1f",RPM_TYP*(1-ERR_MAX/200.));
        M5.Lcd.setCursor(1,129); M5.Lcd.printf("%4.1f",RPM_TYP*(1-ERR_MAX/100.));
        line_x = 27;
      break;
      case 2: // 回転数の数値表示
        M5.Lcd.setRotation(1);     // Rotate the screen. 将屏幕旋转
        M5.Lcd.setTextFont(8);     // 75ピクセルのフォント(数値表示)
        M5.Lcd.fillScreen(BLACK);
      break;
      default:
        M5.Lcd.setRotation(1);     // Rotate the screen. 将屏幕旋转
        M5.Lcd.setTextFont(1);     // 8x6ピクセルのフォント
        M5.Lcd.fillScreen(BLACK);
    }
}

/* After M5StickC Plus is started or reset
  the program in the setUp () function will be run, and this part will only be
  run once. 在 M5StickC Plus
  启动或者复位后，即会开始执行setup()函数中的程序，该部分只会执行一次。 */
void setup() {
    M5.begin();                // Init M5StickC Plus.  初始化 M5StickC Plus
    bh1750Setup(0,26);         // 照度センサのI2Cを初期化
    M5.Imu.Init();             // Init IMU.  初始化IMU
    buf_init();
    lcd_init(0);
    WiFi.mode(WIFI_STA);                        // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
}

/* After the program in setup() runs, it runs the program in loop()
The loop() function is an infinite loop in which the program runs repeatedly
在setup()函数中的程序执行完后，会接着执行loop()函数中的程序
loop()函数是一个死循环，其中的程序会不断的重复运行 */

void loop() {
    M5.update();                                // ボタン状態の取得
    if(M5.BtnA.wasPressed()){                   // (過去に)ボタンが押された時
        disp_mode++;
        if(disp_mode > 3) disp_mode = 0;
        lcd_init(disp_mode);
    }

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
    rpm1 = fabs(gyroZ) / 6.;
    rpm2 = 60 * sqrt(fabs(accY) / 39.478 / RADIUS_CM * 100);
    int rpm1_y = int(67.5 - 67. * (rpm1 - RPM_TYP) / RPM_TYP / ERR_MAX * 100);
    int rpm2_y = int(67.5 - 67. * (rpm2 - RPM_TYP) / RPM_TYP / ERR_MAX * 100);
    int flag_dx = 0;
    
    // モードに応じた処理部（メインボタン押下で切り替え）
    switch(disp_mode){
      case 0:  // 通常モード、水平計＋回転数計
        /**** Gryo ****/
        M5.Lcd.setCursor(0, 4);
        M5.Lcd.println("[Gyro]");
        M5.Lcd.printf(" %5.1f\n %5.1f\n %5.1f\n", gyroX, gyroY, gyroZ);
        
        /**** Accel ****/
        M5.Lcd.println("[Accel]");
        M5.Lcd.printf(" %5.2f\n %5.2f\n %5.2f\n", accX, accY, accZ);
        
        /**** Temperature ****/
    //  M5.Lcd.printf("[Temperature]\n  %6.2f C\n", temp);

        /**** LEVEL ****/
        M5.Lcd.println("[Level]");
        M5.Lcd.printf(" %5.2f\n %5.2f\n", degx, degy);
        M5.Lcd.fillCircle(pos_x_prev[0], pos_y_prev[0], 2, BLACK);
        M5.Lcd.fillCircle(120, 67, 1, DARKGREY);
        for(int i=1; i<BUF_N; i++){
            M5.Lcd.fillCircle(pos_x_prev[i], pos_y_prev[i], 2, DARKGREEN);
        }
        if(pow(degx,2)+pow(degy,2) < pow(DEG_MAX,2)){
            int pos_x = int(64 * degx / DEG_MAX + 0.5) + 120;
            int pos_y = int(64 * degy / DEG_MAX + 0.5) +  67;
            M5.Lcd.fillCircle(pos_x, pos_y, 2, RED);
            buf_append(pos_x_prev, pos_x);
            buf_append(pos_y_prev, pos_y);
        }
        
        /**** RPM ****/
        M5.Lcd.println("[RPM]");
        M5.Lcd.printf(" %5.2f\n %5.2f\n", rpm1, rpm2);
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
        M5.Lcd.setCursor(24, 31);
        M5.Lcd.printf("%04.1f",rpm1);
        break;
      case 3:  // 照度センサを利用した回転数計測
        M5.Lcd.fillRect(0, 4, 240, 16, BLACK);
        M5.Lcd.setCursor(0, 4);
        M5.Lcd.setTextFont(1);     // 8x6ピクセルのフォント
        float lux = getLux();               // 照度(lux)を取得
        M5.Lcd.printf(" Illuminance=%.1f(lx)\n",lux);
        if(lux >= 0.){                      // 正常値の時
            rpm2 = rpm1;
            rpm1 = get_rpm_lum();           // 回転数を測定してrpm1値を置き換え
        }
        M5.Lcd.setTextFont(8);
        M5.Lcd.printf("%4.1f ",rpm1);
    }
    
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
