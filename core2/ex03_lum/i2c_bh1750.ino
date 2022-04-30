/*********************************************************************
本ソースリストおよびソフトウェアは、ライセンスフリーです。(詳細は別記)
利用、編集、再配布等が自由に行えますが、著作権表示の改変は禁止します。

I2C接続の照度センサの値を読み取る
Rohm社 BH1750FVI-TR
                               Copyright (c) 2022 Wataru KUNINO
                               https://bokunimo.net/bokunimowakaru/

【参考文献】
BH1750FVI データシート 2011.11 - Rev.D (ローム)

ADDR = 'L'	010_0011X	0x23

Power Down	0000_0000	0x00	No active state. 
Power On	0000_0001	0x01	
One Time	0010_0001	0x21	H-Resolution Mode2
	Measurement Time is typically 120ms. (max 180ms)

1011100(0)
00100011
max. 24ms.

1011100(1)
*********************************************************************/

#include <Wire.h> 
#define I2C_bh1750 0x23

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

