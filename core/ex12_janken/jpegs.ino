
#include "jpgs/janken0_jpg.h"
#include "jpgs/janken2_jpg.h"
#include "jpgs/janken5_jpg.h"
#include "jpgs/janken8_jpg.h"
#include "jpgs/janken00_jpg.h"
#include "jpgs/janken02_jpg.h"
#include "jpgs/janken05_jpg.h"
#include "jpgs/janken08_jpg.h"
#include "jpgs/janken20_jpg.h"
#include "jpgs/janken22_jpg.h"
#include "jpgs/janken25_jpg.h"
#include "jpgs/janken28_jpg.h"
#include "jpgs/janken50_jpg.h"
#include "jpgs/janken52_jpg.h"
#include "jpgs/janken55_jpg.h"
#include "jpgs/janken58_jpg.h"
#include "jpgs/janken80_jpg.h"
#include "jpgs/janken82_jpg.h"
#include "jpgs/janken85_jpg.h"
#include "jpgs/janken88_jpg.h"
#include "jpgs/janken_jpg.h"

void drawJpgHeadFile(String filename){      // LCDにJPEGファイルを表示する
    switch(filename.length()){
        case 7:     // janken6文字+数字1桁
            switch(filename.substring(6,7).toInt()){
                case 0:
                    M5.Lcd.drawJpg(janken0_jpg, janken0_jpg_len);
                    break;
                case 2:
                    M5.Lcd.drawJpg(janken2_jpg, janken2_jpg_len);
                    break;
                case 5:
                    M5.Lcd.drawJpg(janken5_jpg, janken5_jpg_len);
                    break;
                case 8:
                default:
                    M5.Lcd.drawJpg(janken8_jpg, janken8_jpg_len);
                    break;
            }
            break;
        case 8:     // janken6文字+数字2桁
            switch(filename.substring(6,8).toInt()){
                case 0: // 00
                    M5.Lcd.drawJpg(janken00_jpg, janken00_jpg_len);
                    break;
                case 2: // 02
                    M5.Lcd.drawJpg(janken02_jpg, janken02_jpg_len);
                    break;
                case 5: // 05
                    M5.Lcd.drawJpg(janken05_jpg, janken05_jpg_len);
                    break;
                case 8: // 08
                    M5.Lcd.drawJpg(janken08_jpg, janken08_jpg_len);
                    break;
                case 20:
                    M5.Lcd.drawJpg(janken20_jpg, janken20_jpg_len);
                    break;
                case 22:
                    M5.Lcd.drawJpg(janken22_jpg, janken22_jpg_len);
                    break;
                case 25:
                    M5.Lcd.drawJpg(janken25_jpg, janken25_jpg_len);
                    break;
                case 28:
                    M5.Lcd.drawJpg(janken28_jpg, janken28_jpg_len);
                    break;
                case 50:
                    M5.Lcd.drawJpg(janken50_jpg, janken50_jpg_len);
                    break;
                case 52:
                    M5.Lcd.drawJpg(janken52_jpg, janken52_jpg_len);
                    break;
                case 55:
                    M5.Lcd.drawJpg(janken55_jpg, janken55_jpg_len);
                    break;
                case 58:
                    M5.Lcd.drawJpg(janken58_jpg, janken58_jpg_len);
                    break;
                case 80:
                    M5.Lcd.drawJpg(janken80_jpg, janken80_jpg_len);
                    break;
                case 82:
                    M5.Lcd.drawJpg(janken82_jpg, janken82_jpg_len);
                    break;
                case 85:
                    M5.Lcd.drawJpg(janken85_jpg, janken85_jpg_len);
                    break;
                case 88:
                default:
                    M5.Lcd.drawJpg(janken88_jpg, janken88_jpg_len);
                    break;
            }
            break;
        default:
            M5.Lcd.drawJpg(janken_jpg, janken_jpg_len);
            break;
    }
}
