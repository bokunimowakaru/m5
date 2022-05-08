#include "jpgs/mogura_jpg.h"
#include "jpgs/mogura0_jpg.h"
#include "jpgs/mogura1_jpg.h"
#include "jpgs/mogura2_jpg.h"
#include "jpgs/mogura3_jpg.h"

void drawJpgHeadFile(String filename){      // LCDにJPEGファイルを表示する
    int x = (int)(filename.substring(6,7).toInt()) * 106;
    switch(filename.length()){
        case 6: // mogura 6文字
            M5.Lcd.drawJpg(mogura_jpg, mogura_jpg_len);
            break;
        case 8: // mogura 6文字+数字2桁
            switch(filename.substring(7,8).toInt()){
                case 0:
                    M5.Lcd.drawJpg(mogura0_jpg, mogura0_jpg_len, x, 140);
                    break;
                case 1:
                    M5.Lcd.drawJpg(mogura1_jpg, mogura1_jpg_len, x, 140);
                    break;
                case 2:
                    M5.Lcd.drawJpg(mogura2_jpg, mogura2_jpg_len, x, 140);
                    break;
                case 3:
                    M5.Lcd.drawJpg(mogura3_jpg, mogura3_jpg_len, x, 140);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}
