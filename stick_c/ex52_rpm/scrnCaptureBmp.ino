/*******************************************************************************
M5StickC 用 画面キャプチャ・ツール ※M5.Lcd.readRectRGBが動作しない
********************************************************************************
本ソースは、下記からダウンロードしたソフトウェアを国野亘が改変したものです。
********************************************************************************
    M5Stack 画面キャプチャ・ツール © Marutsuelec Co.,Ltd. All Rights Reserved.
    https://www.marutsu.co.jp/pc/static/large_order/capture_220812
*******************************************************************************/


// M5Stack Screen Capture
// Copyright (c) 2022- @logic_star

#define BitImageSize  (160 * 80 *3)  //画面キャプチャーイメージサイズ
#define PixelPerMeter (160 / 1 * 100) //画面解像度

//Function entry
void getBmpHeader(uint8_t *FrameBuffer){    // 54バイト
    const uint8_t BMP_header[14 + 40] =  //BMPファイルヘッダー定義
      { //File header 14 bytes
        'B','M',    //uint16_t bfType
        (BitImageSize & 0xff),((BitImageSize>>8) & 0xff),((BitImageSize>>16) & 0xff),0,  //uint32_t bfSize
        0,0,        //uint16_t bfReserved1
        0,0,        //uint16_t bfReserved2
        14+40,0,0,0,//uint32_t bfOffBits
        //Information header 40 bytes
        40,0,0,0,   //uint32_t  biSize
        (160 & 0xff),((160 >> 8)& 0xff),0,0,  //int32_t biWidth
        80,0,0,0,                            //int32_t biHeight
        1,0,        //uint16_t biPlanes
        24,0,       //uint16_t biBitCount
        0,0,0,0,    //uint32_t biCompression
        (BitImageSize & 0xff),((BitImageSize>>8) & 0xff),((BitImageSize>>16) & 0xff),0,  //uint32_t biSizeImage
        (PixelPerMeter & 0xff),((PixelPerMeter>>8) & 0xff),0,0,    //int32_t biXPelsPerMeter
        (PixelPerMeter & 0xff),((PixelPerMeter>>8) & 0xff),0,0,    //int32_t biYPelsPerMeter
        0,0,0,0,    //uint32_t biClrUsed
        0,0,0,0 };  //uint32_t biClrImportant
    Serial.printf("getBmpHeader (size=%d)\n", sizeof(BMP_header));
    memcpy(FrameBuffer,BMP_header,54);
}

void getBmpLine(uint8_t *FrameBuffer, int y){    // 240×3バイト
    M5.Lcd.readRectRGB(0, y, 160, 1, FrameBuffer);  //１ライン分の画面データの取得
    int sum = 0;
    uint8_t swap;
    for(int i; i<160*3;i+=3){
        sum += FrameBuffer[i] + FrameBuffer[i+1] + FrameBuffer[i+2];
        swap = FrameBuffer[i];
        FrameBuffer[i] = FrameBuffer[i+2];
        FrameBuffer[i+2] = swap;
    }
    // if(sum == 0)Serial.printf("No Data Y=%d\n",y);
}

/******************************************************************************/
/* 元のソースコード(© Marutsuelec Co.,Ltd. All Rights Reserved.)
// M5Stack Screen Capture
// Copyright (c) 2022- @logic_star

#define BitImageSize  (320 * 240 *3)  //画面キャプチャーイメージサイズ
#define PixelPerMeter (320 / 4 * 100) //画面解像度

//Function entry
void Screen_Capture_BMP(char *file_name){

uint8_t FrameBuffer[320*3];
File    fp;
int     x,y,i;
const uint16_t BMP_header[14 + 40] =  //BMPファイルヘッダー定義
  { //File header 14 bytes
    'B','M',    //uint16_t bfType
    (BitImageSize & 0xff),((BitImageSize>>8) & 0xff),((BitImageSize>>16) & 0xff),0,  //uint32_t bfSize
    0,0,        //uint16_t bfReserved1
    0,0,        //uint16_t bfReserved2
    14+40,0,0,0,//uint32_t bfOffBits
    //Information header 40 bytes
    40,0,0,0,   //uint32_t  biSize
    (320 & 0xff),((320 >> 8)& 0xff),0,0,  //int32_t biWidth
    240,0,0,0,                            //int32_t biHeight
    1,0,        //uint16_t biPlanes
    24,0,       //uint16_t biBitCount
    0,0,0,0,    //uint32_t biCompression
    (BitImageSize & 0xff),((BitImageSize>>8) & 0xff),((BitImageSize>>16) & 0xff),0,  //uint32_t biSizeImage
    (PixelPerMeter & 0xff),((PixelPerMeter>>8) & 0xff),0,0,    //int32_t biXPelsPerMeter
    (PixelPerMeter & 0xff),((PixelPerMeter>>8) & 0xff),0,0,    //int32_t biYPelsPerMeter
    0,0,0,0,    //uint32_t biClrUsed
    0,0,0,0 };  //uint32_t biClrImportant

  Serial.printf("Start %s\n", file_name);
  M5.Lcd.setBrightness(10);       //バックライトを暗くする
  if (!SD.begin(4)) {
    M5.Lcd.println("NO SD CARD.");
    M5.Lcd.setBrightness(200);
    while (1) ;
  }
  fp = SD.open(file_name, FILE_WRITE);
  if(!fp){
    M5.Lcd.println("File open error.");
    M5.Lcd.setBrightness(200);
    while (1) ;
  }
  for(i=0;i<(14+40);i++)  fp.write(BMP_header[i]);  //BMPファイルヘッダーの出力
  for(y=239;y>=0;y--){                              //画面の下から上へスキャン
    M5.Lcd.readRectRGB(0, y, 320, 1, FrameBuffer);  //１ライン分の画面データの取得
    for(x=0;x<320*3;x+=3){        //１ライン分のデータ出力
      fp.write(FrameBuffer[x+2]); //Blue
      fp.write(FrameBuffer[x+1]); //Green
      fp.write(FrameBuffer[x]);   //Red
    }
  }
  fp.close();
  M5.Lcd.setBrightness(200);      //バックライトの明るさを戻す
  Serial.printf("end %s\n", file_name);
}

*/
