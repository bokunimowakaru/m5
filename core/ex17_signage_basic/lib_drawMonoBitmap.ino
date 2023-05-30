// https://github.com/bokunimowakaru/MJ/blob/gh-pages/pg05/bmp/bmp2poke64x128.c

/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
*/

void err(const char *s){
	Serial.printf("ERROR : %s\n",s);
	while(1) delay(1);
}

int drawMonoBitmap(unsigned char *data,int PIX_X, int PIX_Y, int white){
	int i,x,y,p;
	int dX = PIX_X / 8;
	int len = 0, head_n = 0;
	unsigned char c;
	boolean PIX_WHITE = true;
	int PIX_ON = WHITE;
	int PIX_OFF = BLACK;
	if(white == 0){
		PIX_ON = BLACK;
		PIX_OFF = WHITE;
	}else if(white <= 128){
		PIX_ON = WHITE;
		PIX_OFF = LIGHTGREY;  // LIGHTGREY or DARKGREY
	}

	int PIX_SIZE_BYTE = PIX_X * PIX_Y / 8;
	Serial.printf("-- Entered drawMonoBitmap\n");
	
	// Mono BMP形式確認
	if(data[0]!='B' || data[1]!='M'){
		Serial.printf("ERROR: Not BMP\n");
		return 1;
	}
	if(data[0x1C] != 1){
		Serial.printf("ERROR: Not Mono (1bit)\n");
		return 1;
	}
	if((int)data[0x36] + (int)data[0x37] + (int)data[0x38] >= 378){
		PIX_WHITE = !PIX_WHITE;
	}
	
	len = (int)data[2] + (int)data[3]*256;
	Serial.printf("BMP len = %d(0x%02x%02x%02x%02x)\n",len,data[5],data[4],data[3],data[2]);
	if(len < PIX_SIZE_BYTE || data[4] != 0 || data[5] !=0){
		Serial.printf("WARN: BMP Size ERROR \n");
	}
	head_n = (int)data[10] + (int)data[11]*256;
	Serial.printf("Header len = %d(0x%02x%02x%02x%02x)\n",head_n,data[13],data[12],data[11],data[10]);
	
	Serial.printf("## Malloc Heaped\n");
    unsigned char *out = (unsigned char*)malloc(PIX_SIZE_BYTE + 1);
	memset(out,0x00,PIX_SIZE_BYTE + 1);

//	for(i=0;i<64;i++) Serial.printf("-"); Serial.printf("\n");
	
	for(y=0;y<PIX_Y;y++){
		for(x=0;x<dX;x++){
			p = (PIX_Y - y - 1) * dX + x;
			p += head_n;
			out[x + y * dX]=data[p];
		//	Serial.printf("%02x ",out[x+y*8]);
		//	Serial.printf("%04x ",x+PIX_SIZE_BYTE-y*8-8);
		}
		// Serial.printf("\n");
	}
	for(i=0;i<64;i++) printf("-"); printf("\n");
	
	/* 入力データの表示 64 * 64 */
/*	for(y=0;y<64;y++){
		for(x=0;x<64;x++){
			c=data[x/8 + y*dX];
			c=( ( ( c>>(7-(x%8)) ) & 0x01 ) == PIX_WHITE );
			if( c ){
				Serial.printf("#");
			}else{
				Serial.printf(" ");
			}
		}
		Serial.printf("\n");
	}
	for(i=0;i<64;i++) printf("-"); printf("\n");
*/

	/* 文字回転処理 */
/*	memcpy(data,out,PIX_SIZE_BYTE);
	memset(out,0x00,PIX_SIZE_BYTE);
	for(y=0;y<PIX_Y;y++){
		for(x=0;x<PIX_X;x++){
			c=data[x/8+y*8];
			c=( ( ( c>>(7-(x%8)) ) & 0x01 ) == PIX_WHITE );
			if( c ){
				out[x/8+(y/8)*64+(x%8)*8] |= ( 0x01<<(y%8));
			}
		}
	}
*/	
	
	/* 先頭のデータが0xFFだとIchigoJam側でファイルとして扱われない */
/*	c=out[(PIX_X/8)-1];
	if(!PIX_WHITE) c = ~c;
	if(c==0xFF) c=0xEF;
	if(!PIX_WHITE) c = ~c;
	out[(PIX_X/8)-1]=c;
*/
	/* 出力用のデータ表示 64 * 64 */
/*	for(y=0;y<64;y++){
		for(x=0;x<64;x++){
			c=out[x/8+y*dX];
			if( (( c>>(7-(x%8)) ) & 0x01) == PIX_WHITE ){
				Serial.printf("#");
			}else{
				Serial.printf(" ");
			}
		}
		Serial.printf("\n");
	}
	for(i=0;i<64;i++) printf("-"); printf("\n");
*/

	/* データ出力 */
	for(y=0;y<PIX_Y;y++){
		for(x=0;x<PIX_X;x++){
			c=out[x/8+y*dX];
			if( (( c>>(7-(x%8)) ) & 0x01) == PIX_WHITE ){
				M5.Lcd.drawLine(x, y, x, y, PIX_ON);
			}else{
				M5.Lcd.drawLine(x, y, x, y, PIX_OFF);
			}
		}
	}
	for(i=0;i<64;i++) printf("-"); printf("\n");

	free(out);
	Serial.printf("## Malloc Released\n");
	Serial.printf("-- Finished drawMonoBitmap\n");
	return 0;
}

int drawMonoBitmap(unsigned char *out,int PIX_X, int PIX_Y){
	return drawMonoBitmap(out,PIX_X,PIX_Y,128);
}

int drawMonoBitmapFile(const char *filename,int PIX_X, int PIX_Y, int white){
	int i,x,y,p;
	int dX = PIX_X / 8;
	int len = 0, head_n = 0;
	unsigned char c;
	boolean PIX_WHITE = true;
	int PIX_ON = WHITE;
	int PIX_OFF = BLACK;
	if(white == 0){
		PIX_ON = BLACK;
		PIX_OFF = WHITE;
	}else if(white <= 128){
		PIX_ON = WHITE;
		PIX_OFF = LIGHTGREY;  // LIGHTGREY or DARKGREY
	}

	int PIX_SIZE_BYTE = PIX_X * PIX_Y / 8;
	Serial.printf("-- Entered drawMonoBitmapFile\n");
	
	File file;
	unsigned char data[8];

	Serial.printf("開始(%s)\n",filename);
//	Serial.printf("PIX_WHITE = %02x\n",PIX_WHITE);
	Serial.printf("入力ファイル確認\n");
	file = SPIFFS.open(filename, "r");
	len = file.available();
	if(len == 0) err("cannot open file; len = 0");

	// Mono BMP形式確認
	file.read(data, 2);
	if(data[0]!='B' || data[1]!='M'){
		Serial.printf("ERROR: Not BMP\n");
		return 1;
	}
	file.seek(0x1C,SeekSet);
	file.read(&c, 1);

	if(c != 1){
		Serial.printf("ERROR: Not Mono (1bit)\n");
		return 1;
	}
	file.seek(0x36,SeekSet);
	file.read(data, 3);
	if((int)data[0] + (int)data[1] + (int)data[2] >= 378){
		PIX_WHITE = !PIX_WHITE;
	}

	file.seek(0x00,SeekSet);
	file.read(data, 6);
	len = (int)data[2] + (int)data[3]*256;
	Serial.printf("BMP len = %d(0x%02x%02x%02x%02x)\n",len,data[5],data[4],data[3],data[2]);
	if(len < PIX_SIZE_BYTE || data[4] != 0 || data[5] !=0){
		Serial.printf("WARN: BMP Size ERROR \n");
	}
	file.seek(10,SeekSet);
	file.read(data, 4);
	head_n = (int)data[0] + (int)data[1]*256;
	Serial.printf("Header len = %d(0x%02x%02x%02x%02x)\n",head_n,data[3],data[2],data[1],data[0]);
	
	Serial.printf("## Malloc Heaped\n");
    unsigned char *out = (unsigned char*)malloc(PIX_SIZE_BYTE + 1);
	memset(out,0x00,PIX_SIZE_BYTE + 1);

	for(y=0;y<PIX_Y;y++){
		for(x=0;x<dX;x++){
			p = (PIX_Y - y - 1) * dX + x;
			p += head_n;
			file.seek(p,SeekSet);
			file.read(&c, 1);
			out[x + y * dX]=c;
		}
	}
	for(y=0;y<PIX_Y;y++){
		for(x=0;x<PIX_X;x++){
			c=out[x/8+y*dX];
			if( (( c>>(7-(x%8)) ) & 0x01) == PIX_WHITE ){
				M5.Lcd.drawLine(x, y, x, y, PIX_ON);
			}else{
				M5.Lcd.drawLine(x, y, x, y, PIX_OFF);
			}
		}
	}
	file.close();
	free(out);
	Serial.printf("## Malloc Released\n");
	Serial.printf("-- Finished drawMonoBitmapFile\n");
	return 0;
}

int drawMonoBitmapFile(const char *filename){
	return drawMonoBitmapFile(filename, 320, 240, 128);
}

int drawMonoBitmapFile(const char *filename, int white){
	return drawMonoBitmapFile(filename, 320, 240, white);
}

/*
~/m5/tools $ ./bmp2header.py html/mono01.bmp
./bmp2header.py
input filename  = html/mono01.bmp
output filename = html/mono01_bmp.h
0x42, 0x4D, 0xC0, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x28, 0x00,
0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x0B, 0x00, 0x00, 0x12, 0x0B, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
Format type  : BM
File size    : 9664 [byte] , len = 9664
Header size  : 62 [byte] --> 0x3E OK
Width        : 320 [px]
Height       : 240 [px]
Total pixels : 76800
Color        : 1 [bit]
Done
*/

/*
000000 42 4d c0 25 00 00 00 00 00 00 3e 00 00 00 28 00  >BM.%......>...(.<
000010 00 00 40 01 00 00 f0 00 00 00 01 00 01 00 00 00  >..@.............<
000020 00 00 00 00 00 00 12 0b 00 00 12 0b 00 00 00 00  >................<
000030 00 00 00 00 00 00 ff ff ff 00 00 00 00 00 40 81  >..............@.<
000040 12 08 02 41 00 00 01 58 25 2b 69 75 bb 55 6c 5a  >...A...X%+iu.UlZ<

000000 42 4d cf 03 00 00 00 00 00 00 3e 00 00 00 28 00  >BM........>...(.<  ファイルサイズに誤り
000010 00 00 40 01 00 00 f0 00 00 00 01 00 01 00 00 00  >..@.............<
000020 00 00 80 25 00 00 00 00 00 00 00 00 00 00 00 00  >...%............<
000030 00 00 00 00 00 00 00 00 00 00 ff ff ff 00 ff ff  >................<
000040 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff  >................<

000000 42 4d c0 25 00 00 00 00 00 00 3e 00 00 00 28 00  >BM.%......>...(.<
000010 00 00 40 01 00 00 f0 00 00 00 01 00 01 00 00 00  >..@.............<
000020 00 00 00 00 00 00 12 0b 00 00 12 0b 00 00 00 00  >................<
000030 00 00 00 00 00 00 ff ff ff 00 00 00 00 00 00 00  >................<
000040 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  >................<
*/
