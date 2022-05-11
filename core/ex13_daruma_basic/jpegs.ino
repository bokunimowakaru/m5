
#include "jpgs/daruma0_jpg.h"
#include "jpgs/daruma2_jpg.h"
#include "jpgs/daruma3_jpg.h"
#include "jpgs/daruma4_jpg.h"

void drawJpgHeadFile(String filename,int x, int y){      // LCDにJPEGファイルを表示する
    switch(filename.length()){
        case 7:     // daruma6文字+数字1桁
            switch(filename.substring(6,7).toInt()){
                case 2:
                    M5.Lcd.drawJpg(daruma2_jpg, daruma2_jpg_len, x, y);
                    break;
                case 3:
                    M5.Lcd.drawJpg(daruma3_jpg, daruma3_jpg_len, x, y);
                    break;
                case 4:
                    M5.Lcd.drawJpg(daruma4_jpg, daruma4_jpg_len, x, y);
                    break;
                case 0:
                default:
                    M5.Lcd.drawJpg(daruma0_jpg, daruma0_jpg_len, x, y);
                    break;
            }
            break;
        default:
            break;
    }
}

void drawJpgHeadFile(String filename){
    int x=0,y=0;
    switch(filename.length()){
        case 7:     // daruma6文字+数字1桁
            switch(filename.substring(6,7).toInt()){
                case 2:
                    x = 80;
                    y = 32;
                    break;
                case 3:
                    x = 0;
                    y = 0;
                    break;
                case 4:
                    x = 172;
                    y = 8;
                    break;
                case 0:
                default:
                    x = 0;
                    y = 0;
                    break;
            }
            break;
        default:
            x = 0;
            y = 0;
            break;
    }
    drawJpgHeadFile(filename,x, y);
}
