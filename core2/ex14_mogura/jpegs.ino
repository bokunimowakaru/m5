#include "jpgs/mogura_jpg.h"
#include "jpgs/mogura0_jpg.h"
#include "jpgs/mogura1_jpg.h"
#include "jpgs/mogura2_jpg.h"
#include "jpgs/mogura3_jpg.h"
#include "jpgs/mogura4_jpg.h"
#include "jpgs/mogura5_jpg.h"

void drawJpgHeadFile(String filename,int x, int y){      // LCDにJPEGファイルを表示する
    switch(filename.length()){
        case 6: // mogura 6文字
            M5.Lcd.drawJpg(mogura_jpg, mogura_jpg_len);
            break;
        case 7: // mogura 6文字+数字1桁
            switch(filename.substring(6,7).toInt()){
                case 0:
                    M5.Lcd.drawJpg(mogura0_jpg, mogura0_jpg_len, x, y);
                    break;
                case 1:
                    M5.Lcd.drawJpg(mogura1_jpg, mogura1_jpg_len, x, y);
                    break;
                case 2:
                    M5.Lcd.drawJpg(mogura2_jpg, mogura2_jpg_len, x, y);
                    break;
                case 3:
                    M5.Lcd.drawJpg(mogura3_jpg, mogura3_jpg_len, x, y);
                    break;
                case 4:
                    M5.Lcd.drawJpg(mogura4_jpg, mogura4_jpg_len, x, y);
                    break;
                case 5:
                    M5.Lcd.drawJpg(mogura5_jpg, mogura5_jpg_len, x, y);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

void drawJpgHeadFile(String filename,int x){
    int y=0;
    if(filename.length() >= 7) y=140;
    drawJpgHeadFile(filename,x, y);
}

void drawJpgHeadFile(String filename){
    int x=0,y=0;
    if(filename.length() >= 7){
        x=106;
        y=140;
    }
    drawJpgHeadFile(filename,x, y);
}
