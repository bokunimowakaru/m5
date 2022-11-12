/*********************************************************************
本ソースリストおよびソフトウェアは、ライセンスフリーです。(詳細は別記)
利用、編集、再配布等が自由に行えますが、著作権表示の改変は禁止します。

I2C接続の温湿度センサの値を読み取る
SENSIRION社 SHT31

【ｓM5Stack用 BTC SHT30用】

							   Copyright (c) 2017-2020 Wataru KUNINO
							   https://bokunimo.net/bokunimowakaru/
*********************************************************************/

#include <Wire.h> 
#define I2C_sht 0x44			// SHT30 の I2C アドレス 

volatile float _i2c_sht31_hum = -999;
volatile float _i2c_sht31_temp = -999;

float i2c_sht30_getTemp(){
	int temp,hum;
	if( _i2c_sht31_temp >= -100 ){
		float ret;
		ret = _i2c_sht31_temp;
		_i2c_sht31_temp = -999;
		return ret;
	}
	_i2c_sht31_hum=-999.;
	_i2c_sht31_temp = -999;
	Wire.beginTransmission(I2C_sht);
	Wire.write(0x2C);
	Wire.write(0x06);
	delay(18);					// 15ms以上
	if( Wire.endTransmission() == 0){
		Wire.requestFrom(I2C_sht,6);
		if(Wire.available()==0) return -999.;
		temp = Wire.read();
		temp <<= 8;
		if(Wire.available()==0) return -999.;
		temp += Wire.read();
		if(Wire.available()==0) return -999.;
		Wire.read();
		if(Wire.available()==0) return -999.;
		hum = Wire.read();
		hum <<= 8;
		if(Wire.available()==0) return -999.;
		hum += Wire.read();
		if(Wire.available()==0) return -999.;
		Wire.read();
		_i2c_sht31_hum = (float)hum / 65535. * 100.;
		return (float)temp / 65535. * 175. - 45.;
	}else return -999.;
}

float i2c_sht30_getHum(){
	float ret;
	if( _i2c_sht31_hum < 0) _i2c_sht31_temp = i2c_sht30_getTemp();
	ret = _i2c_sht31_hum;
	_i2c_sht31_hum = -999;
	return ret;
}

uint16_t i2c_sht30_getStat(){
	uint16_t stat=0x00;
	Wire.beginTransmission(I2C_sht);
	Wire.write(0xF3);
	Wire.write(0x2D);
	if( Wire.endTransmission() == 0){
		Wire.requestFrom(I2C_sht,3);
		if(Wire.available()==0) return 0xFFFF;
		stat = (uint16_t)Wire.read();
		stat <<= 8;
		if(Wire.available()==0) return 0xFFFF;
		stat += (uint16_t)Wire.read();
		Wire.read();
	}
	delay(18);
	Wire.beginTransmission(I2C_sht);
	Wire.write(0x30);
	Wire.write(0x41);
	Wire.endTransmission();
	return stat;
}

void i2c_sht30_printStat(){
	uint16_t stat=i2c_sht30_getStat();
	Serial.printf("i2c_sht31_printStat = 0x%04x\n", stat );
	Serial.printf("Alert pending status  : %01d\n", (stat>>15) & 0x1 );
	Serial.printf("Heater status         : %01d\n", (stat>>13) & 0x1 );
	Serial.printf("RH tracking alert     : %01d\n", (stat>>11) & 0x1 );
	Serial.printf("T tracking alert      : %01d\n", (stat>>10) & 0x1 );
	Serial.printf("System reset detected : %01d\n", (stat>>4) & 0x1 );
	Serial.printf("Command status        : %01d\n", (stat>>1) & 0x1 );
	Serial.printf("Write data checksum   : %01d\n", stat & 0x1 );
}

boolean i2c_sht30_Setup(int PIN_SDA = 21, int PIN_SCL = 22);

boolean i2c_sht30_Setup(int PIN_SDA, int PIN_SCL){
	delay(2);					// 1ms以上
	boolean ret = Wire.begin(PIN_SDA,PIN_SCL); // I2Cインタフェースの使用を開始
	delay(18);					// 15ms以上
	return ret;
}
