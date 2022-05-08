
#include "jpgs/daruma0_jpg.h"
#include "jpgs/daruma2_jpg.h"
#include "jpgs/daruma3_jpg.h"
#include "jpgs/daruma4_jpg.h"

void drawJpgHeadFile(String filename){      // LCDにJPEGファイルを表示する
    switch(filename.length()){
        case 7:     // daruma6文字+数字1桁
            switch(filename.substring(6,7).toInt()){
                case 2:
                    M5.Lcd.drawJpg(daruma2_jpg, daruma2_jpg_len, 80, 32);
                    break;
                case 3:
                    M5.Lcd.drawJpg(daruma3_jpg, daruma3_jpg_len);
                    break;
                case 4:
                    M5.Lcd.drawJpg(daruma4_jpg, daruma4_jpg_len, 172, 8);
                    break;
                case 0:
                default:
                    M5.Lcd.drawJpg(daruma0_jpg, daruma0_jpg_len);
                    break;
            }
            break;
        default:
            break;
    }
}
