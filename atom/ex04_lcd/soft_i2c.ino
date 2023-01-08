/*******************************************************************************
Raspberry Pi用 ソフトウェアI2C ライブラリ  raspi_i2c / soft_i2c
Arduino ESP32 用 ソフトウェア I2C LCD ドライバ soft_i2c

本ソースリストおよびソフトウェアは、ライセンスフリーです。(詳細は別記)
利用、編集、再配布等が自由に行えますが、著作権表示の改変は禁止します。

Arduino標準ライブラリ「Wire」は使用していない(I2Cの手順の学習用サンプル)

										Copyright (c) 2014-2023 Wataru KUNINO
										https://bokunimo.net/raspi/
							 			https://bokunimo.net/
********************************************************************************
元ファイル：
https://github.com/bokunimowakaru/RaspberryPi/blob/master/libs/soft_i2c.c
********************************************************************************
最新ファイル：
https://bokunimo.net/git/raspi_lcd/blob/master/raspi_i2c.h
*******************************************************************************/

//	通信の信頼性確保のため、戻り値の仕様を変更・統一しました。
//	ヘッダファイルも変更しています。ご理解のほど、お願いいたします。
//	0:成功 1:失敗
//														2017/6/16	国野亘

#ifndef ARDUINO // Raspberry Pi, Linux
	#include <stdio.h>
	#include <stdlib.h>
	#include <stdint.h>						// uint32_t
	#include <unistd.h> 					// usleep用
	#include <ctype.h>						// isprint用
	#include <sys/time.h>					// gettimeofday用
	#include <string.h>						// strncpy用
#endif

// #define RASPI_GPIO //【動作速度が、かなり遅い】

#define I2C_lcd 0x3E						// LCD の I2C アドレス

#ifndef ARDUINO // Raspberry Pi, Linux
	#define PORT_SCL	"/sys/class/gpio/gpio3/value"		// I2C SCLポート
	#define PORT_SDA	"/sys/class/gpio/gpio2/value"		// I2C SDAポート
	#define PORT_SDANUM	2									// I2C SDAポートの番号
															// SCLはSDA+1(固定)
	#define INPUT		"in"
	#define OUTPUT		"out"
	#define LOW			0
	#define HIGH		1
#endif

#ifdef ARDUINO
	#define	I2C_RAMDA	30				// I2C データシンボル長[us]
#else  // Raspberry Pi, Linux
	#define	I2C_RAMDA	15				// I2C データシンボル長[us]
#endif
#define GPIO_RETRY	50					// GPIO 切換え時のリトライ回数
#define S_NUM		16					// 文字列の最大長
// #define DEBUG			   			// デバッグモード
#undef DEBUG
// #define DEBUG_UTF8					// UTF8デバッグモード

#ifdef ARDUINO
	byte PORT_SCL = 22;								// I2C SCLポート
	byte PORT_SDA = 21;								// I2C SDAポート
	unsigned long micros_prev;
#else  // Raspberry Pi, Linux
	typedef unsigned char byte; 
	FILE *fgpio;
	char buf[S_NUM];
	struct timeval micros_time;				//time_t micros_time;
	int micros_prev,micros_sec;
#endif
int ERROR_CHECK=1;								// 1:ACKを確認／0:ACKを無視する
static byte _lcd_size_x=8;
static byte _lcd_size_y=2;

int _micros(){
	#ifdef ARDUINO
		unsigned long micros_sec=micros();
		if( micros_prev < micros_sec ) return micros_sec - micros_prev;
		return ( UINT_MAX - micros_prev ) + micros_sec;
	#else  // Raspberry Pi, Linux
		int micros;
		gettimeofday(&micros_time, NULL);	 // time(&micros_time);
		micros = micros_time.tv_usec;
		if(micros_prev > micros ) micros_sec++;
		micros_prev = micros;
		micros += micros_sec * 1000000;
		return micros;
	#endif
}

void _micros_0(){
	#ifdef ARDUINO
		micros_prev=micros();
	#else  // Raspberry Pi, Linux
		micros_sec=0;
	#endif
}

void _delayMicroseconds(int i){
	#ifdef ARDUINO
		delayMicroseconds(i);
	#else  // Raspberry Pi, Linux
		usleep(i);
	#endif
}

#ifndef ARDUINO // Raspberry Pi, Linux, Cygwin
void delay(int i){
	while(i){
		_delayMicroseconds(1000);
		i--;
	}
}
#endif

void i2c_debug(const char *s,byte priority){
	#ifdef ARDUINO
		#ifdef DEBUG
		   	Serial.print(_micros());
			if(priority>3) Serial.print(" ERROR:"); else Serial.print("      :");
			Serial.println(s);
		#endif
	#else // Raspberry Pi, Linux
		if(priority>3)	fprintf(stderr,"[%10d] ERROR:%s\n",_micros(),s);
		#ifdef DEBUG
		else 			fprintf(stderr,"[%10d]      :%s\n",_micros(),s);
		#endif
	#endif
}

void i2c_error(const char *s){
	i2c_debug(s,5);
}
void i2c_log(const char *s){
	i2c_debug(s,1);
}

#ifndef ARDUINO // Raspberry Pi, Linux, Cygwin
byte pinMode(char *port, char *mode){
// 戻り値：０の時はエラー
  #ifdef RASPI_GPIO
	char com_i[]="/usr/bin/raspi-gpio set 3 ip pu 2>/dev/null";
			   // 01234567890123456789012345678901234567890123	44 bytes
	char com_o[]="/usr/bin/raspi-gpio set 3 op 2>/dev/null";
			   // 01234567890123456789012345678901234567890 	41 bytes
	com_i[24]=port[20];	// ポート番号は1桁にしか対応していない
	com_o[24]=port[20];	// ポート番号は1桁にしか対応していない
	if(mode[0] == INPUT[0]){
		#ifdef DEBUG
		//	fprintf(stderr,"pinMode %s\n",com_i);
		#endif
		if(!system(NULL)){
			i2c_error("I2C_pinMode INPUT / system call Error");
		}else{
			system(com_i);
		}
	}
	if(mode[0] == OUTPUT[0]){
		#ifdef DEBUG
		//	fprintf(stderr,"pinMode %s\n",com_o);
		#endif
		if(!system(NULL)){
			i2c_error("I2C_pinMode OUTPUT / system call Error");
		}else{
			system(com_o);
		}
	}
	return 1;
  #else
	int i=0;
	char dir[]="/sys/class/gpio/gpio3/direction";
			 // 01234567890123456789012345678901	22 Bytes
	dir[20]=port[20];
	#ifdef DEBUG
	//	fprintf(stderr,"pinMode %s %s\n",dir,mode);
	#endif
	while(i<GPIO_RETRY){
		fgpio = fopen(dir, "w");
		if(fgpio){
			fprintf(fgpio,mode);
			fclose(fgpio);
			return 1;
		}
		delay(1);
		i++;
	}
	#ifdef DEBUG
	//	fprintf(stderr,"pinMode / GPIO_RETRY (%d/%d)\n",i,GPIO_RETRY);
	#endif
	return 0;
  #endif
}
#endif

#ifndef ARDUINO // Raspberry Pi, Linux
byte digitalRead(char *port){
// 戻り値：読み値。エラー時は０（将来 -１に変更するかもしれない）
  #ifdef RASPI_GPIO
	FILE *pp;
	char buf[48];
			// GPIO 3: level=1 fsel=0 func=INPUT
			// 0123456789012345678901234567890122 34 bytes
	char com[]="/usr/bin/raspi-gpio get 3 2> /dev/null";
			//	012345678901234567890123456789012345678  39 bytes
	com[24]=port[20];	// ポート番号は1桁にしか対応していない
	#ifdef DEBUG
	//	fprintf(stderr,"digitalRead %s, ",com);
	#endif
	pp=popen(com,"r");
	if(pp){
		fgets(buf,47,pp);
		pclose(pp);
		#ifdef DEBUG
		//	fprintf(stderr,"%s, %d\n",buf,(byte)atoi(&buf[14]));
		#endif
		if(strncmp(buf,"GPIO ",5) || strlen(buf) < 15){
			i2c_error("I2C_digitalRead / IO Input Error");
			return 0;
		}
		return (byte)atoi(&buf[14]);
	}
	return 0;
  #else
	fgpio = fopen(port, "r");
	if( fgpio ){
		fgets(buf, S_NUM, fgpio);
		fclose(fgpio);
	}
	#ifdef DEBUG
	//	fprintf(stderr,"digitalRead %s %s\n",port,buf);
	#endif
  #endif
	return (byte)atoi(buf);
}
#endif

#ifndef ARDUINO // Raspberry Pi, Linux
byte digitalWrite(char *port, int value){
// 戻り値：０の時はエラー
  #ifdef RASPI_GPIO
	char com[]="/usr/bin/raspi-gpio set 3 dl 2>/dev/null";
			 // 01234567890123456789012345678901234567890  41 bytes
	com[24]=port[20];	// ポート番号は1桁にしか対応していない
	if(value) com[27]='h';
	#ifdef DEBUG
	//	fprintf(stderr,"digitalWrite %s\n",com);
	#endif

	if(!system(NULL)){
		i2c_error("I2C_digitalWrite / system call Error");
	}else{
		system(com);
		return 1; // OK
	}
	return 0;  // エラー
  #else
	fgpio = fopen(port, "w");
	if( fgpio ){
		fprintf(fgpio,"%d\n",value);
		fclose(fgpio);
		return 1;
	}
	#ifdef DEBUG
	//	fprintf(stderr,"digitalWrite %s %d\n",port,value);
	#endif
	return 0;  // エラー
  #endif
}
#endif

#ifndef ARDUINO // Raspberry Pi, Linux
byte i2c_hard_reset(int port){
	// 戻り値：０の時はエラー
  #ifdef RASPI_GPIO  // 動作未確認
	char com[]="/usr/bin/raspi-gpio set 00 dl 2>/dev/null";
			//	012345678901234567890123456789012345678901	42 bytes
	if(port<1 || port>99) return 0;
	com[24] = '\0';
	sprintf(com,"%s%2d dl 2>/dev/null",com,port);
	#ifdef DEBUG
		printf("%s\n",com);
	#endif
	if(!system(NULL)){
		i2c_error("I2C_RESET(L) / system call Error");
		return 0; // error
	}else{
		system(com);
	}
	
	delay(10);
	com[24] = '\0';
	sprintf(com,"%s%2d dh 2>/dev/null",com,port);
	#ifdef DEBUG
		printf("%s\n",com);
	#endif
	if(!system(NULL)){
		i2c_error("I2C_RESET(H) / system call Error");
		return 0; // error
	}else{
		system(com);
	}
	delay(10);
	return 1;
  #else
	char dir[]="/sys/class/gpio/gpio00/direction";
			 // 012345678901234567890123456789012	 33 Bytes
	char com[]="/sys/class/gpio/gpio00/value";
			 // 01234567890123456789012345678		 29 Bytes
	// int i;
	if(port<10 || port>99) return 0; // error
	dir[20]=(char)(port/10) + '0';	// port 1～9には対応しない
	dir[21]=(char)(port%10) + '0';
	com[20]=(char)(port/10) + '0';	// port 1～9には対応しない
	com[21]=(char)(port%10) + '0';
	
	// GPIO初期化処理
	#ifdef DEBUG
	//	fprintf(stderr,"I2C_RESET IO Settiong %s out\n",dir);
	#endif
	fgpio = fopen("/sys/class/gpio/export","w");
	if(fgpio==NULL ){
		i2c_error("I2C_RESET / IO Settiong Error\n");
		return 0;
	}
	fprintf(fgpio,"%d\n",port);
	fclose(fgpio);
	delay(10);

	// pinMode = out 設定処理
	#ifdef DEBUG
	//	fprintf(stderr,"I2C_RESET pinMode %s out\n",dir);
	#endif
	fgpio = fopen(dir, "w");
	if(fgpio){
		fprintf(fgpio,"out");
		fclose(fgpio);
	}else{
		i2c_error("I2C_RESET i2c_hard_reset");
		i2c_error(dir);
		i2c_error("the port is already used, or some hardware problems");
		i2c_error("try: raspi-gpio set (port) ip");
		return 0;
	}
	delay(1);

	// digitalWrite = 0, Lレベル設定処理
	#ifdef DEBUG
	//	fprintf(stderr,"I2C_RESET digitalWrite %s %d\n",com,0);
	#endif
	fgpio = fopen(com, "w");
	if( fgpio ){
		fprintf(fgpio,"%d\n",0);
		fclose(fgpio);
	}else{
		i2c_error("I2C_RESET fopen i2c_hard_reset (L)");
		return 0;
	}
	delay(10);
	
	// digitalWrite = 1, Hレベル設定処理
	#ifdef DEBUG
	//	fprintf(stderr,"I2C_RESET digitalWrite %s %d\n",com,1);
	#endif
	fgpio = fopen(com, "w");
	if( fgpio ){
		fprintf(fgpio,"%d\n",1);
		fclose(fgpio);
	}else{
		i2c_error("I2C_RESET fopen i2c_hard_reset (H)");
		return 0;
	}
	delay(10);
	return 0; // error
  #endif
//##############################################################################
	/* RaspberryPi/gpio 使用(古い)
	FILE *pp;
	char buf[9];
	char com[]="/home/pi/RaspberryPi/gpio/raspi_gpo 00 0 &> /dev/null";
			//	012345678901234567890123456789012345678901234567890123	54 bytes
	if(port<1 || port>99) return 0;
	com[36] = '\0';
	sprintf(com,"%s%2d 0 &> /dev/null",com,port);
//	#ifdef DEBUG
		printf("%s\n",com);
//	#endif
	pp=popen(com,"r");
	if(pp){
		fgets(buf,8,pp);
		pclose(pp);
		if(buf[0] != '0'){
			i2c_error("I2C_RESET(L) / IO Settiong Error");
			return 0;
		}
	}
	delay(10);
	com[36] = '\0';
	sprintf(com,"%s%2d 1 &> /dev/null",com,port);
	#ifdef DEBUG
		printf("%s\n",com);
	#endif
	pp=popen(com,"r");
	if(pp){
		fgets(buf,8,pp);
		pclose(pp);
		if(buf[0] != '1'){
			i2c_error("I2C_RESET(H) / IO Settiong Error");
			return 0;
		}
	}
	delay(10);
	*/
}
#endif

#ifdef ARDUINO
void i2c_SCL(byte level){
	if( level ){
		pinMode(PORT_SCL, INPUT);
	}else{
		pinMode(PORT_SCL, OUTPUT);
		digitalWrite(PORT_SCL, LOW);
	}
	_delayMicroseconds(I2C_RAMDA);
}
void i2c_SDA(byte level){
	if( level ){
		pinMode(PORT_SDA, INPUT);
	}else{
		pinMode(PORT_SDA, OUTPUT);
		digitalWrite(PORT_SDA, LOW);
	}
	_delayMicroseconds(I2C_RAMDA);
}
#else
byte i2c_SCL(byte level){
// 戻り値：０の時はエラー
	byte ret=0;
	if( level ){
		ret += !pinMode(PORT_SCL, INPUT);
	}else{
		ret += !pinMode(PORT_SCL, OUTPUT);
		ret += !digitalWrite(PORT_SCL, LOW);
	}
	_delayMicroseconds(I2C_RAMDA);
	return !ret;
}
byte i2c_SDA(byte level){
// 戻り値：０の時はエラー
	byte ret=0;
	if( level ){
		ret += !pinMode(PORT_SDA, INPUT);
	}else{
		ret += !pinMode(PORT_SDA, OUTPUT);
		ret += !digitalWrite(PORT_SDA, LOW);
	}
	_delayMicroseconds(I2C_RAMDA);
	return !ret;
}
#endif

byte i2c_tx(const byte in){
// 戻り値：０の時はエラー
	int i;
	#ifdef DEBUG
		char s[32];
		sprintf(s,"tx data = [%02X]",in);
		i2c_log(s);
	#endif
	for(i=0;i<8;i++){
		if( (in>>(7-i))&0x01 ){
				i2c_SDA(1);					// (SDA)	H Imp
		}else	i2c_SDA(0);					// (SDA)	L Out
		/*Clock*/
		i2c_SCL(1);							// (SCL)	H Imp
		i2c_SCL(0);							// (SCL)	L Out
	}
	/* ACK処理 */
	_delayMicroseconds(I2C_RAMDA);
	i2c_SDA(1);								// (SDA)	H Imp  2016/6/26 先にSDAを終わらせる
	i2c_SCL(1);								// (SCL)	H Imp
	if(!ERROR_CHECK){
		_delayMicroseconds(I2C_RAMDA);
	}else{
		for(i=3;i>0;i--){						// さらにクロックを上げた瞬間には確定しているハズ
			if( digitalRead(PORT_SDA) == 0 ) break;	// 速やかに確認
			_delayMicroseconds(I2C_RAMDA/2);
		}
		if(i==0){
			i2c_SCL(0);							// (SCL)	L Out
			i2c_log("no ACK");
			return 0;
		}
	}
	#ifdef DEBUG
	//	fprintf(stderr,"i2c_tx / GPIO_RETRY (%d/%d)\n",GPIO_RETRY-i,GPIO_RETRY);
	#endif
	return (byte)i;
}

byte i2c_init(void){
// 戻り値：０の時はエラー
	int i;

	_micros_0();
	i2c_log("I2C_Init");
	#ifndef ARDUINO  // ### for Raspberry Pi
		for(i=0;i<2;i++){
			fgpio = fopen("/sys/class/gpio/export","w");
			if(fgpio==NULL ){
				i2c_error("I2C_Init / IO Settiong Error\n");
				printf("9\n");
				return 0;
			}
			fprintf(fgpio,"%d\n",i + PORT_SDANUM);
			fclose(fgpio);
		}
	#endif
	for(i=GPIO_RETRY;i>0;i--){						// リトライ50回まで
		i2c_SDA(1);							// (SDA)	H Imp
		i2c_SCL(1);							// (SCL)	H Imp
		if( digitalRead(PORT_SCL)==1 &&
			digitalRead(PORT_SDA)==1  ) break;
		delay(1);
	}
	if(i==0) i2c_error("I2C_Init / Locked Lines");
	#ifdef DEBUG
	//	fprintf(stderr,"i2c_init / GPIO_RETRY (%d/%d)\n",GPIO_RETRY-i,GPIO_RETRY);
	#endif
	_delayMicroseconds(I2C_RAMDA*8);
	return (byte)i;
}

byte i2c_close(void){
// 戻り値：０の時はエラー
	byte i;
	i2c_log("i2c_close");
	#ifndef ARDUINO  // ### for Raspberry Pi
		for(i=0;i<2;i++){
			fgpio = fopen("/sys/class/gpio/unexport","w");
			if(fgpio==NULL ){
				fprintf(stderr,"IO Error\n");
				printf("9\n");
				return 0;
			}
			fprintf(fgpio,"%d\n",i + PORT_SDANUM);
			fclose(fgpio);
		}
	#endif
	return 1;
}

byte i2c_start(void){
// 戻り値：０の時はエラー
//	if(!i2c_init())return(0);				// SDA,SCL	H Out
	int i;

	for(i=GPIO_RETRY*100;i>0;i--){			// リトライ 50×100ms
		i2c_SDA(1);							// (SDA)	H Imp
		i2c_SCL(1);							// (SCL)	H Imp
		if( digitalRead(PORT_SCL)==1 &&
			digitalRead(PORT_SDA)==1  ) break;
		delay(1);
	}
	i2c_log("i2c_start");
	if(i==0 && ERROR_CHECK) i2c_error("i2c_start / Locked Lines");
	_delayMicroseconds(I2C_RAMDA*8);
	i2c_SDA(0);								// (SDA)	L Out
	_delayMicroseconds(I2C_RAMDA);
	i2c_SCL(0);								// (SCL)	L Out
	return (byte)i;
}

byte i2c_check(byte adr){
/*
入力：byte adr = I2Cアドレス(7ビット)
戻り値：０の時はエラー
*/
	byte ret;
	if( !i2c_start() ) {
		i2c_error("i2c_check / aborted i2c_start");
		return 0;
	}
	adr <<= 1;								// 7ビット->8ビット
	adr &= 0xFE;							// RW=0 送信モード
	ret=i2c_tx(adr);

	/* STOP */
	i2c_SDA(0);								// (SDA)	L Out
	i2c_SCL(0);								// (SCL)	L Out
	_delayMicroseconds(I2C_RAMDA);
	i2c_SCL(1);								// (SCL)	H Imp
	_delayMicroseconds(I2C_RAMDA);
	i2c_SDA(1);								// (SDA)	H Imp
	return ret;
}


byte i2c_read(byte adr, byte *rx, byte len){
/*
入力：byte adr = I2Cアドレス(7ビット)
出力：byte *rx = 受信データ用ポインタ
入力：byte len = 受信長
戻り値：byte 受信結果長、０の時はエラー
*/
	byte ret,i;
	
	if( !i2c_start() && ERROR_CHECK) return 0;
	adr <<= 1;								// 7ビット->8ビット
	adr |= 0x01;							// RW=1 受信モード
	if( i2c_tx(adr)==0 && ERROR_CHECK ){	// アドレス設定
		i2c_error("I2C_RX / no ACK (Address)");
		return 0;		
	}
	
	/* スレーブ待機状態待ち */
	for(i=GPIO_RETRY;i>0;i--){
		_delayMicroseconds(I2C_RAMDA);
		if( digitalRead(PORT_SDA)==0  ) break;
	}
	if(i==0 && ERROR_CHECK){
		i2c_error("I2C_RX / no ACK (Reading)");
		return 0;
	}
	for(i=10;i>0;i--){
		_delayMicroseconds(I2C_RAMDA);
		if( digitalRead(PORT_SCL)==1  ) break;
	}
	if(i==0 && ERROR_CHECK){
		i2c_error("I2C_RX / Clock Line Holded");
		return 0;
	}
	/* 受信データ */
	for(ret=0;ret<len;ret++){
		i2c_SCL(0);							// (SCL)	L Out
		i2c_SDA(1);							// (SDA)	H Imp
		rx[ret]=0x00;
		for(i=0;i<8;i++){
			i2c_SCL(1);						// (SCL)	H Imp
			rx[ret] |= (digitalRead(PORT_SDA))<<(7-i);		//data[22] b4=Port 12(SDA)
			i2c_SCL(0);						// (SCL)	L Out
		}
		if(ret<len-1){
			// ACKを応答する
			i2c_SDA(0);							// (SDA)	L Out
			i2c_SCL(1);							// (SCL)	H Imp
			_delayMicroseconds(I2C_RAMDA);
		}else{
			// NACKを応答する
			i2c_SDA(1);							// (SDA)	H Imp
			i2c_SCL(1);							// (SCL)	H Imp
			_delayMicroseconds(I2C_RAMDA);
		}
	}
	/* STOP */
	i2c_SCL(0);								// (SCL)	L Out
	i2c_SDA(0);								// (SDA)	L Out
	_delayMicroseconds(I2C_RAMDA);
	i2c_SCL(1);								// (SCL)	H Imp
	_delayMicroseconds(I2C_RAMDA);
	i2c_SDA(1);								// (SDA)	H Imp
	return ret;
}

byte i2c_write(byte adr, byte *tx, byte len){
/*
入力：byte adr = I2Cアドレス(7ビット)
入力：byte *tx = 送信データ用ポインタ
入力：byte len = 送信データ長（0のときはアドレスのみを送信する）
戻り値：０の時はエラー(または送信データ長0)
*/
	byte ret=0;
	if( !i2c_start() ) return 0;
	adr <<= 1;								// 7ビット->8ビット
	adr &= 0xFE;							// RW=0 送信モード
	if( i2c_tx(adr)>0 ){
		/* データ送信 */
		for(ret=0;ret<len;ret++){
			i2c_SDA(0);						// (SDA)	L Out
			i2c_SCL(0);						// (SCL)	L Out
			if( i2c_tx(tx[ret]) == 0 && ERROR_CHECK){
				i2c_error("i2c_write / no ACK (Writing)");
				return 0;
			}
		}
	}else if( len>0 && ERROR_CHECK){		// len=0の時はエラーにしないAM2320用
		i2c_error("i2c_write / no ACK (Address)");
		return 0;
	}
	/* STOP */
	i2c_SDA(0);								// (SDA)	L Out
	i2c_SCL(0);								// (SCL)	L Out
	_delayMicroseconds(I2C_RAMDA);
	if(len==0)_delayMicroseconds(800);		// AM2320用
	i2c_SCL(1);								// (SCL)	H Imp
	_delayMicroseconds(I2C_RAMDA);
	i2c_SDA(1);								// (SDA)	H Imp
	return ret;
}

byte i2c_lcd_out(byte y,byte *lcd){
// 戻り値：０の時はエラー
	#ifdef I2C_LCD_OFF
		Serial.println((char *)lcd);
		return 1;
	#endif
	byte data[2];
	byte i;
	byte ret=0;

	data[0]=0x00;
	if(y==0) data[1]=0x80;
	else{
		data[1]=0xC0;
		y=1;
	}
	ret += !i2c_write(I2C_lcd,data,2);
	for(i=0;i<_lcd_size_x;i++){
		if(lcd[i]==0x00) break;
		data[0]=0x40;
		data[1]=lcd[i];
		ret += !i2c_write(I2C_lcd,data,2);
	}
	#ifdef DEBUG
		if(ret)fprintf(stderr,"ERROR LOD_OUT Y=%d [%s]\n",y,lcd);
	#endif
	return !ret;
}

const char _utf_C3_x80x90xE0[]={
	0x87, 0xBC, 0xA9, 0xA2, 0xA4, 0xA0, 0xA5, 0xA7,
	0xAA, 0xAB, 0xA8, 0xAF, 0xAE, 0xAC, 0x84, 0x85,
	0x89, 0xA6, 0x86, 0xB4, 0xB6, 0xB2, 0xBB, 0xB9,
	0xBF, 0x96, 0x9C, 0xB1, 0x91, 0x2F, 0x2F, 0x2F,
	0xA1, 0xAD, 0xB3, 0xBA, 0x2F, 0x2F, 0x2F, 0x2F,
	0x2F, 0x2F, 0x83, 0xA3, 0x95, 0xB5, 0x98, 0xB8,
	0x00
};

void utf_del_uni(char *s){
	byte i=0;
	byte j=0;
	char k;
	char *p;
	#ifdef DEBUG_UTF8
		fprintf(stderr,"in > ");
		while(s[i]!='\0'){
			fprintf(stderr,"%02X ",s[i]);
			i++;
		}
		fprintf(stderr,"len=%d\n",i+1);
		i=0;
		fprintf(stderr,"out> ");
	#endif
	while(s[i]!='\0'){
		if((byte)s[i]==0xEF){
			if((byte)s[i+1]==0xBE) s[i+2] += 0x40;
			i+=2;
		}else if((byte)s[i]==0xC3) {	// 2バイト ラテン文字
			i+=1;
			p=strchr(_utf_C3_x80x90xE0, (int)s[i]);
			if(p){
				k = (char)(p - _utf_C3_x80x90xE0);
				if(k < 0x20) s[i] = (char)(k + 0x80);
				else if(k < 0x30) s[i] = (char)(k + 0xC0);
				else i -= 1;
			}else{ // 小文字で代用
				if((byte)s[i]==0x80)	  s[i]=0xA0;
				else if((byte)s[i]==0x82) s[i]=0xA2;
				else if((byte)s[i]==0x88) s[i]=0xA8;
				else if((byte)s[i]==0x8A) s[i]=0xAA;
				else if((byte)s[i]==0x8B) s[i]=0xAB;
				else if((byte)s[i]==0x8E) s[i]=0xAE;
				else if((byte)s[i]==0x8F) s[i]=0xAF;
				else if((byte)s[i]==0x94) s[i]=0xB4;
				else if((byte)s[i]==0x9A) s[i]=0xBA;
				else if((byte)s[i]==0x9B) s[i]=0xBB;
				p=strchr(_utf_C3_x80x90xE0, (int)s[i]);
				if(p){
					k = (char)(p - _utf_C3_x80x90xE0);
					if(k < 0x20) s[i] = (char)(k + 0x80);
					else if(k < 0x30) s[i] = (char)(k + 0xC0);
					else i -= 1;
				}
			}
		}else if((byte)s[i]==0xC2){
			if((byte)s[i+1]==0xBF)		{i++; s[i]=0x9F;}
			else if((byte)s[i+1]==0xA2) {i++; s[i]=0xE4;}
			else if((byte)s[i+1]==0xA3) {i++; s[i]=0xE5;}
			else if((byte)s[i+1]==0xA7) {i++; s[i]=0xE8;}
		}else if((byte)s[i]==0xC4 && (byte)s[i+1]==0xB1){
			i++; s[i]=0xE9;
		}else if((byte)s[i]==0xC5 && (byte)s[i+1]==0x93){
			// oe
			s[i]='o';
			s[j]=s[i];
			j++;
			s[i+1]='e';
			i++;
		}else if((byte)s[i]==0xC5 && (byte)s[i+1]==0x92){
			// OE
			s[i]='O';
			s[j]=s[i];
			j++;
			s[i+1]='E';
			i++;
		}
		#ifdef DEBUG_UTF8
			fprintf(stderr,"%02X ",s[i]);
		#endif
		if(isprint(s[i]) || ((byte)s[i]>=0x80 && (byte)s[i] <=0xEF)){
			s[j]=s[i];
			j++;
		}
		i++;
	}
	s[j]='\0';
	#ifdef DEBUG_UTF8
		fprintf(stderr,"len=%d\n",j);
	#endif
}

	byte i2c_lcd_print(const char *s);

byte i2c_lcd_init(void){
// 戻り値：０の時はエラー
	#ifdef I2C_LCD_OFF
		return;
	#endif
	byte ret=0;
	byte data[2];

	data[0]=0x00; data[1]=0x39; ret+=!i2c_write(I2C_lcd,data,2);	// IS=1
	
	data[0]=0x00; data[1]=0x14; ret+=!i2c_write(I2C_lcd,data,2);	// OSC=4
//	data[0]=0x00; data[1]=0x11; ret+=!i2c_write(I2C_lcd,data,2);	// OSC=1

	data[0]=0x00; data[1]=0x73; ret+=!i2c_write(I2C_lcd,data,2);	// コントラスト	0x3
//	data[0]=0x00; data[1]=0x70; ret+=!i2c_write(I2C_lcd,data,2);	// コントラスト	0x0

	data[0]=0x00; data[1]=0x5E; ret+=!i2c_write(I2C_lcd,data,2);	// Power/Cont	0xE
//	data[0]=0x00; data[1]=0x56; ret+=!i2c_write(I2C_lcd,data,2);	// Power/Cont	0x6

	data[0]=0x00; data[1]=0x6C; ret+=!i2c_write(I2C_lcd,data,2);	// FollowerCtrl	0xC

	delay(200);

	data[0]=0x00; data[1]=0x38; ret+=!i2c_write(I2C_lcd,data,2);	// IS=0

	data[0]=0x00; data[1]=0x0C; ret+=!i2c_write(I2C_lcd,data,2);	// DisplayON	0xC

//	i2c_lcd_print("Hello!  I2C LCD by Wataru Kunino");
	return !ret;
}

byte i2c_lcd_init_xy(byte x, byte y){
// 戻り値：０の時はエラー
	#ifdef I2C_LCD_OFF
		return 1;
	#endif
	if(x==16||x==8||x==20) _lcd_size_x=x;
	if(y==1 ||y==2) _lcd_size_y=y;
	return i2c_lcd_init();
}

void i2c_lcd_set_xy(byte x, byte y){
	#ifdef I2C_LCD_OFF
		return;
	#endif
	if(x==16||x==8||x==20) _lcd_size_x=x;
	if(y==1 ||y==2) _lcd_size_y=y;
}

#ifdef ARDUINO
void i2c_lcd_init_xy_sdascl(byte x,byte y,byte sda,byte scl){
	if(x==16||x==8||x==20) _lcd_size_x=x;
	if(y==1 ||y==2) _lcd_size_y=y;
	PORT_SCL = scl;
	PORT_SDA = sda;
	i2c_lcd_init();
}
#endif

byte i2c_lcd_print(const char *s){
// 戻り値：０の時はエラー
	byte i,j;
	char str[97];
	byte lcd[21];
	byte ret=0;

	strncpy(str,s,96);
	str[96]='\0';
	utf_del_uni(str);
	for(j=0;j<2;j++){
		lcd[_lcd_size_x]='\0';
		for(i=0;i<_lcd_size_x;i++){
			lcd[i]=(byte)str[i+_lcd_size_x*j];
			if(lcd[i]==0x00){
				for(;i<_lcd_size_x;i++) lcd[i]=' ';
				ret += !i2c_lcd_out(j,lcd);
				if(j==0){
					for(i=0;i<_lcd_size_x;i++) lcd[i]=' ';
					ret += !i2c_lcd_out(1,lcd);
				}
				return !ret;
			}
		}
		ret += !i2c_lcd_out(j,lcd);
	}
	return !ret;
}

byte i2c_lcd_print2(const char *s){
// 戻り値：０の時はエラー
	byte ret=0;
	byte i;
	char str[97];
	byte lcd[21];
	
	strncpy(str,s,96);
	str[96]='\0';
	utf_del_uni(str);
	lcd[_lcd_size_x]='\0';
	for(i=0;i<_lcd_size_x;i++){
		lcd[i]=(byte)str[i];
		if(lcd[i]==0x00){
			for(;i<_lcd_size_x;i++) lcd[i]=' ';
			ret += !i2c_lcd_out(1,lcd);
			return !ret;
		}
	}
	ret += !i2c_lcd_out(1,lcd);
	return !ret;
}


byte i2c_lcd_print_ip(uint32_t ip){
// 戻り値：０の時はエラー
	char lcd[21];
	
	if(_lcd_size_x<=8){
		snprintf(lcd,21,"%i.%i.    ",
			ip & 255,
			ip>>8 & 255
		);
		snprintf(&lcd[8],21,"%i.%i",
			ip>>16 & 255,
			ip>>24
		);
	}else{
		snprintf(lcd,21,"%i.%i.%i.%i",
			ip & 255,
			ip>>8 & 255,
			ip>>16 & 255,
			ip>>24
		);
	}
	return i2c_lcd_print(lcd);
}

byte i2c_lcd_print_ip2(uint32_t ip){
// 戻り値：０の時はエラー
	char lcd[21];
	
	snprintf(lcd,21,"%i.%i.%i.%i",
		ip & 255,
		ip>>8 & 255,
		ip>>16 & 255,
		ip>>24
	);
	if(_lcd_size_x>=16) return i2c_lcd_print2(lcd);
	else return i2c_lcd_print(lcd);
}

byte i2c_lcd_print_val(const char *s,int in){
// 戻り値：０の時はエラー
	byte ret=0;
	char lcd[21];
	snprintf(lcd,21,"%d",in);
	ret += !i2c_lcd_print(s);
	ret += !i2c_lcd_print2(lcd);
	return !ret;
}

/*******************************************************************************

time2txt 用に使用したライブラリの権利情報：

time.c - low level time and date functions
Copyright (c) Michael Margolis 2009

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

6  Jan 2010 - initial release 
12 Feb 2010 - fixed leap year calculation error
1  Nov 2010 - fixed setTime bug (thanks to Korman for this)
*******************************************************************************/
/*============================================================================*/	
/* functions to convert to and from system time */
/* These are for interfacing with time serivces and are not normally needed in a sketch */
// leap year calulator expects year argument as years offset from 1970
//	static	const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};
// API starts months from 1, this array starts from 0
//	void breakTime(time_t time, tmElements_t &tm){
	// break the given time_t into time components
	// this is a more compact version of the C library localtime function
	// note that year is offset from 1970 !!!
#define LEAP_YEAR(Y)	 ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )
void time2txt(char *date,unsigned long local){
	int Year,year;
	int Month,month, monthLength;
	int Day;
	int Second,Minute,Hour;
//	int Wday;  // Sunday is day 1 
	unsigned long days;
	static	const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};
	Second = local % 60;
	local /= 60; // now it is minutes
	Minute = local % 60;
	local /= 60; // now it is hours
	Hour = local % 24;
	local /= 24; // now it is days
//	Wday = ((local + 4) % 7) + 1;  // Sunday is day 1 
	year = 0;  
	days = 0;
	while((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= local) {
		year++;
	}
//	Year = year; // year is offset from 1970 
	days -= LEAP_YEAR(year) ? 366 : 365;
	local  -= days; // now it is days in this year, starting at 0
	days=0;
	month=0;
	monthLength=0;
	for (month=0; month<12; month++) {
		if (month==1) { // february
			if (LEAP_YEAR(year)) {
				monthLength=29;
			} else {
				monthLength=28;
			}
		} else {
			monthLength = monthDays[month];
		}

		if (local >= (unsigned long)monthLength) {
			local -= (unsigned long)monthLength;
		} else {
			break;
		}
	}
	Year = year + 1970;
	Month = month + 1;	// jan is month 1  
	Day = local + 1;	 // day of month
	snprintf(date,20,"%4d/%02d/%02d,%02d:%02d:%02d",Year,Month,Day,Hour,Minute,Second);
	// warning: ‘%02d’ directive output may be truncated writing 2 bytes into a region of size between 0 and 3 [-Wformat-truncation=]
	// %やabsで制限すれば回避できるが、誤表示となる解決にはならない。コード上で(ほぼ)配慮されているので無視。
}

byte i2c_lcd_print_time(unsigned long local){
// 戻り値：０の時はエラー
	byte ret=0;
	char date[20];	//	0123456789012345678
					//	2014/01/01,12:34:56
	
	time2txt(date,local);
	if(_lcd_size_x<=8){
		date[10]='\0';
		ret += !i2c_lcd_print(&date[2]);
		ret += !i2c_lcd_print2(&date[11]);
	}else if(_lcd_size_x>=19){
		ret += !i2c_lcd_print(date);
	}else if(_lcd_size_x>=10){
		date[10]='\0';
		ret += !i2c_lcd_print(date);
		ret += !i2c_lcd_print2(&date[11]);
	}
	return !ret;
}

/******************************************************************************/
/* トランジスタ技術 2016.6 ESP-WROOM-02特集記事用 I2C LCD ライブラリ 互換 API */
/*																			  */
/*										Copyright (c) 2014-2019 Wataru KUNINO */
/******************************************************************************/

#ifdef ARDUINO

void lcdOut(byte y,byte *lcd){
	i2c_lcd_out(y,lcd);
}

void lcdPrint(const char *s){
	i2c_lcd_print(s);
}

void lcdPrint(String s){
	char lcd[97];								// 表示用変数を定義(97バイト96文字)
	int len;									// 文字列長を示す整数型変数を定義
	memset(lcd, 0, 97); 						// 文字列変数lcdの初期化(97バイト)
	s.toCharArray(lcd, 97);
	i2c_lcd_print(lcd);
}

void lcdPrint2(const char *s){
	i2c_lcd_print2(s);
}

void lcdPrintIp(uint32_t ip){
	i2c_lcd_print_ip(ip);
}

void lcdPrintIp2(uint32_t ip){
	i2c_lcd_print_ip2(ip);
}

void lcdPrintVal(const char *s,int in){
	i2c_lcd_print_val(s,in);
}

void lcdPrintTime(unsigned long local){
	i2c_lcd_print_time(local);
}

void lcdSetup(byte x, byte y, byte sda,byte scl){
	i2c_lcd_init_xy_sdascl(x,y,sda,scl);
}

void lcdSetup(byte x, byte y){
	i2c_lcd_init_xy(x,y);
}

void lcdSetup(){
	i2c_lcd_init();
}
#endif
