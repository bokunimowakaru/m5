/******************************************************************************
BH1750FVI-TRを回転させたときの周期を求めます

                                               Copyright (c) 2024 Wataru KUNINO
*******************************************************************************
設計事項
・2周期分以上のサンプルを取得し、時系列データの自己相関ピーク点から周期を求める
・精度33.3 rpm ±0.1rpm を計測するには 333×2周期分以上のサンプルが必要になる。
・測定範囲を±10%すると、733サンプルの中に2周期分が入る必要がある
・保持するサンプル数を800とし、サンプル間隔は測定で合わせる。5.4msが目標
　∵サンプルレートは185サンプル/秒（1回転1.8秒、333サンプルなので）

******************************************************************************/

#include <Wire.h> 
#define I2C_bh1750 0x23

int calculate_autocorrelation_peak_lag(uint16_t data[], int n);

uint16_t lux_array[800];

float get_rpm_lum(){
    Wire.beginTransmission(I2C_bh1750);
    Wire.write(0x13);            // Continuously Resolution Mode (L=0x13,H=0x10)
    Wire.endTransmission(false); // send a restart, keeping the connection active.
    delay(180);                  // 16ms以上 ローレゾ24ms ハイレゾ180ms

    int cnt;
    M5.Lcd.printf(" Sampling:");
    unsigned long start = millis();
    for(int i=0;i<800;i++){
        Wire.requestFrom(I2C_bh1750, 2, false);
        for(cnt=0; cnt < 1000; cnt++) { // 1 second waiting time max
            if(Wire.available())break;
            delay(1);
        }
        if(cnt == 1000){
            M5.Lcd.printf("\nBH1750FVI is not ready (i=%d,byte=0)\n",i);
            delay(3000);
            return 0;
        }
        lux_array[i] = Wire.read() << 8;
        for(cnt=0; cnt < 1000; cnt++) { // 1 second waiting time max
            if(Wire.available())break;
            delay(1);
        }
        if(cnt == 1000){
            M5.Lcd.printf("\nBH1750FVI is not ready (i=%d,byte=1)\n",i);
            delay(3000);
            return 0;
        }
        lux_array[i] += Wire.read();
        if(i%80 == 0) M5.Lcd.print('.');
        delay(5);
    }
    unsigned long end = millis();
    float msPerSample = (float)(end - start)/800.;
    M5.Lcd.printf("\n SamplingTime=%d(ms)/800(samples) \n", end - start);
    M5.Lcd.printf(" %d(sample/sec), %.1f(ms/sample) \n", 800000/(end - start), msPerSample);
    Wire.endTransmission(true);
    int lag = calculate_autocorrelation_peak_lag(lux_array, 800);
    
    float dist = (float)lag * msPerSample;
    float rpm = round(600000. / dist) / 10.;
    M5.Lcd.printf(" Lag=%d, Dist=%.0f(ms), RPM=%.1f(rpm) \n", lag, dist, rpm);
    return rpm;
}

/*********************************************************************
本ソースリストおよびソフトウェアは、ライセンスフリーです。(詳細は別記)
利用、編集、再配布等が自由に行えますが、著作権表示の改変は禁止します。

I2C接続の照度センサの値を読み取る
Rohm社 BH1750FVI-TR
                               Copyright (c) 2022 Wataru KUNINO
                               https://bokunimo.net/bokunimowakaru/

【参考文献】
BH1750FVI データシート 2011.11 - Rev.D (ローム)

ADDR = 'L'  010_0011X   0x23

Power Down  0000_0000   0x00    No active state. 
Power On    0000_0001   0x01    
One Time    0010_0001   0x21    H-Resolution Mode2
    Measurement Time is typically 120ms. (max 180ms)

1011100(0)
00100011
max. 24ms.

1011100(1)
*********************************************************************/

float getLux(){
    int lux;
    Wire.beginTransmission(I2C_bh1750);
    Wire.write(0x21);
    if( Wire.endTransmission() == 0){
        delay(180);                  // 180ms以上
        Wire.requestFrom(I2C_bh1750,2);
        if(Wire.available()==0) return -999.;
        lux = Wire.read();
        lux <<= 8;
        if(Wire.available()==0) return -999.;
        lux += Wire.read();
        return (float)lux / 1.2;
    }else return -999.;
}

void bh1750Setup(int sda,int scl){
    delay(1);
    Wire.begin(sda,scl);        // I2Cインタフェースの使用を開始
    delay(25);                  // 25ms以上
}

void bh1750Setup(){
    Wire.begin();               // I2Cインタフェースの使用を開始
    delay(18);                  // 15ms以上
}

void bh1750Power(int on){
    if(on){
        Wire.beginTransmission(I2C_bh1750);
        Wire.write(0x01);
        Wire.endTransmission();
    }else{
        Wire.beginTransmission(I2C_bh1750);
        Wire.write(0x00);
        Wire.endTransmission();
    }
}

/******************************************************************************
calculate_autocorrelation
*******************************************************************************
ChatGPTが生成したソースコードをWataru KUNINOが改変したものです
プロンプト：
自己相関を求め、相関のピークが現れるラグ値を得るプログラムをC言語で作ってください
******************************************************************************/

// (1)入力をuint16_tに修正し、(2)ラグの範囲をn/3以上に制限、(3)32bit FPU対応

int calculate_autocorrelation_peak_lag(uint16_t data[], int n) { //.........(1)
    float max_autocorrelation = -1.0; // 最小の初期値を設定
    int peak_lag = 0; // ピークが現れるラグ値を保存

    for (int k = n/3; k < n; k++) { //......................................(2)
        float sum = 0.0; //.................................................(3)
        for (int t = 0; t < n - k; t++) {
            sum += (float)data[t] * (float)data[t + k];
        }
        float autocorrelation = sum / (n - k);
        if (autocorrelation > max_autocorrelation) {
            max_autocorrelation = autocorrelation;
            peak_lag = k;
        }
    }
    return peak_lag;
}

// (以下、Chat GPTが生成) 
/*
// 自己相関を計算し、ピークが現れるラグ値を返す関数
int calculate_autocorrelation_peak_lag(double data[], int n) {
    double max_autocorrelation = -1.0; // 最小の初期値を設定
    int peak_lag = 0; // ピークが現れるラグ値を保存

    for (int k = 0; k < n; k++) {
        double sum = 0.0;
        for (int t = 0; t < n - k; t++) {
            sum += data[t] * data[t + k];
        }
        double autocorrelation = sum / (n - k);
        if (autocorrelation > max_autocorrelation) {
            max_autocorrelation = autocorrelation;
            peak_lag = k;
        }
    }
    return peak_lag;
}

int main() {
    int n;

    // データの長さを入力
    printf("データの長さを入力してください: ");
    scanf("%d", &n);

    double data[n];

    // データの入力
    printf("データを入力してください:\n");
    for (int i = 0; i < n; i++) {
        printf("data[%d] = ", i);
        scanf("%lf", &data[i]);
    }

    // 自己相関のピークが現れるラグ値を計算
    int peak_lag = calculate_autocorrelation_peak_lag(data, n);

    // 結果を表示
    printf("自己相関のピークが現れるラグ値: %d\n", peak_lag);

    return 0;
}
*/
