/*********************************************************************

赤外線リモコン送信部 for Arduino Version 2.0

本ソースリストおよびソフトウェアは、ライセンスフリーです。
個人での利用は自由に行えます。著作権表示の改変は禁止します。

                               Copyright (c) 2012-2022 Wataru KUNINO
                               https://bokunimo.net/bokunimowakaru/
*********************************************************************/
/*
赤外線リモコン信号を送信します。
*/

#define IR_OUT_OFF	LOW				// 赤外線LED非発光時の出力値
#define IR_OUT_ON	HIGH			// 赤外線LED発光時の出力値
#define DATA_SIZE	16				// データ長(byte),4の倍数、16以上


#define FLASH_AEHA_TIMES	16	// シンボルの搬送波点滅回数(AEHA)400us 
#define FLASH_NEC_TIMES		25	// シンボルの搬送波点滅回数(NEC) 600us 22 -> 25 2022/10
#define FLASH_SIRC_TIMES	26	// シンボルの搬送波点滅回数(SIRC)600us 24 -> 26 2022/10
/*
#define FLASH_AEHA_TIMES	17	// シンボルの搬送波点滅回数(AEHA) 16 -> 17 2022/1/30 ESP32C3
#define FLASH_NEC_TIMES		23	// シンボルの搬送波点滅回数(NEC)  22 -> 23 2022/1/30 ESP32C3
#define FLASH_SIRC_TIMES	25	// シンボルの搬送波点滅回数(SIRC) 24 -> 25 2022/1/30 未確認+1
*/
#define FLASH_ON			11	// LED ON 期間 us (規格上 ON+OFFで 23 us) -> 実測24us
#define FLASH_OFF			11	// LED ON 期間 us (規格上 ON+OFFで 23 us)

// enum IR_TYPE{ AEHA=0, NEC=1, SIRC=2 };		// 家製協AEHA、NEC、SONY SIRC切り換え
#define AEHA		0
#define NEC			1
#define SIRC		2

#define DEBUG_ARDUINO

int _PIN_IR_OUT = 2;                 // 赤外線LEDの接続ポート

void ir_send_init(void){
	pinMode(_PIN_IR_OUT, OUTPUT);
	digitalWrite(_PIN_IR_OUT, IR_OUT_OFF);
}

void ir_send_init(int port){
	pinMode(port, OUTPUT);
	digitalWrite(port, IR_OUT_OFF);
	_PIN_IR_OUT = port;
}

/* 赤外線ＬＥＤ点滅用 */
void ir_flash(byte times){
	while(times){
		times--;
		delayMicroseconds(FLASH_ON);
		digitalWrite(_PIN_IR_OUT, IR_OUT_ON);
		delayMicroseconds(FLASH_OFF);
		digitalWrite(_PIN_IR_OUT, IR_OUT_OFF);
	}
}
void ir_wait(byte times){
	while(times){
		times--;
		delayMicroseconds(FLASH_ON);
		digitalWrite(_PIN_IR_OUT, IR_OUT_OFF);
		delayMicroseconds(FLASH_OFF);
		digitalWrite(_PIN_IR_OUT, IR_OUT_OFF);
	}
}

#ifndef _ahex2i
#define _ahex2i
int ahex2i(char c){
    if(c>='0' && c<='9') return c-'0';
    if(c>='a' && c<='f') return c-'a'+10;
    if(c>='A' && c<='F') return c-'A'+10;
    return -1;
}
#endif

/* 信号変換 */
int ir_txt2data(byte *data, int max, char *txt){
	//	byte *data	IR用バイトデータ出力
	//	int max		最大バイト数
	//	char *txt	IR用テキストデータ入力
	//	戻り値		出力データ長
	int data_i,len,in;
	
	len=atoi(txt);
	txt=strstr(txt,",");
	if(!txt) return 0;
	data_i=0;
	while(data_i<max){
		txt++;
		if( *txt=='\0' ) break;
		in = ahex2i(*txt);
		if(in>=0){
			data[data_i]=(byte)(in<<4);
			txt++;
			in = ahex2i(*txt);
			if(in<0) return 0;		// Null検出もここで行っている
			data[data_i] += (byte)in;
			data_i++;
		}
	}
	#ifdef DEBUG_ARDUINO
		Serial.print("data_i=");Serial.println(data_i);
		Serial.print(len);
		for(int i=0;i<data_i;i++){
			Serial.print(",");
			Serial.print(data[i]>>4,HEX);
			Serial.print(data[i]&15,HEX);
		}
		Serial.println();
	#endif
	return len;
}
int ir_data2txt(char *txt, int txt_max, byte *data, int data_len){
	//	char *txt		IR用テキストデータ出力
	//	int txt_max		最大文字数( txt[max+1] )
	//	byte *data		IR用バイトデータ入力
	//	int data_len	IRデータのビット長
	//	戻り値			出力文字長
	int i,len;
	int data_max=data_len/8;
	
	if(data_len%8)data_max++;
	i=0;
	if( txt_max <= 3*data_max ) return -1;
	for(len=0;len<data_max;len++){
		if(i>0) txt[i-1]=',';
		txt[i]  =(char)(data[len]>>4)+'0'; if( txt[i]  >'9' ) txt[i]   += ('A'-'9') - 1;
		txt[i+1]=(char)(data[len]&15)+'0'; if( txt[i+1]>'9' ) txt[i+1] += ('A'-'9') - 1;
		txt[i+2]='\0';
		i+=3;
		if( i+2 >= txt_max) break;
	}
	if(len==data_max)len--;
	return len;
}

/* 赤外線ＬＥＤ信号送出 */
void ir_send(byte *data, const byte data_len, const byte ir_type ){
	byte i,j,k,t;
	byte b;
	
	if(data_len<16) return;

    portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;  // 排他制御用
    portENTER_CRITICAL_ISR(&mutex);             // 割り込みの禁止
	switch( ir_type ){
		case NEC:
			ir_flash( 8 * FLASH_NEC_TIMES );	// send 'SYNC_H'
			ir_flash( 8 * FLASH_NEC_TIMES );
			ir_wait(  8 * FLASH_NEC_TIMES );	// send 'SYNC_L'
			for( i = 0 ; i < data_len/8 ; i++){
				for( b = 0 ; b < 8 ; b++ ){
					ir_flash( FLASH_NEC_TIMES );
					if( data[i] & (0x01 << b) ){
						ir_wait( 3 * FLASH_NEC_TIMES );
					}else{
						ir_wait( FLASH_NEC_TIMES );
					}
				}
			}
			ir_flash( FLASH_NEC_TIMES );		// send 'stop'
			break;
		case SIRC:
			for(j=0;j<3;j++){
				t=5;						// 送信済シンボル基本単位(SYNCで送信)
				ir_flash( 4 * FLASH_SIRC_TIMES );	// send 'SYNC_H'
				ir_wait(      FLASH_SIRC_TIMES );	// send 'SYNC_L'
				for( b = 0 ; b < 7 ; b++ ){
					if( data[0] & (0x01 << b) ){
						ir_flash( 2 * FLASH_SIRC_TIMES );
						t +=3 ;
					}else{
						ir_flash( FLASH_SIRC_TIMES );
						t +=2 ;
					}
					ir_wait( FLASH_SIRC_TIMES );
				}
				// for( i = 8 ; i < (data_len/8+(data_len%8!=0)) ; i++){
				for( k = 8 ; k < data_len ; k++){
					i = k / 8;
					b = k % 8;
					if( data[i] & (0x01 << b) ){
						ir_flash( 2 * FLASH_SIRC_TIMES );
						t +=3 ;
					}else{
						ir_flash( FLASH_SIRC_TIMES );
						t +=2 ;
					}
					ir_wait( FLASH_SIRC_TIMES );
				}
				while( t <= 75 ){
					t++;
					ir_wait( FLASH_SIRC_TIMES );
				}
			}
			break;
		case AEHA:
		default:
			ir_flash( 8 * FLASH_AEHA_TIMES );	// send 'SYNC_H'
			ir_wait(  4 * FLASH_AEHA_TIMES);	// send 'SYNC_L'
			for( i = 0 ; i < data_len/8 ; i++){
				for( b = 0 ; b < 8 ; b++ ){
					ir_flash( FLASH_AEHA_TIMES );
					if( data[i] & (0x01 << b) ){
						ir_wait( 3 * FLASH_AEHA_TIMES );
					}else{
						ir_wait( FLASH_AEHA_TIMES );
					}
				}
			}
			ir_flash( FLASH_AEHA_TIMES );		// send 'stop'
			break;
	}
    portEXIT_CRITICAL_ISR(&mutex);              // 割り込み許可
}

void ir_send(byte *data, const byte data_len, const byte ir_type, byte repeat){
	if(ir_type == SIRC) repeat = (repeat+2)/3; // ÷=3 の切り上げ処理
	if(repeat == 0) repeat = 1;
	for(byte i=0;i<repeat;i++){
		unsigned long t = millis();
		ir_send(data, data_len, ir_type);
		unsigned long dt = millis() - t;
		unsigned long weight = 0;
		if(i < repeat - 1){
			switch( ir_type ){
				case NEC:	
					weight = 108 - dt;
					break;
				case SIRC:
					weight = 45 - dt;
					break;
				case AEHA:
				default:
					weight = 130 - dt;
				break;
			}
			if(weight > 80 ) weight = 80;
			if(weight < 8 ) weight = 8;
			delay(weight);
		}
	}
}
