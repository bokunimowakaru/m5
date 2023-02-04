#include "jpgs/wtr_clud_jpg.h"
#include "jpgs/wtr_fine_jpg.h"
#include "jpgs/wtr_rain_jpg.h"
#include "jpgs/wtr_snow_jpg.h"
#include "jpgs/wtr_uknw_jpg.h"

void drawJpgHeadFile(String filename,int x, int y){      // LCDにJPEGファイルを表示する
    if(filename.equals("wtr_clud_jpg")){
        M5.Lcd.drawJpg(wtr_clud_jpg, wtr_clud_jpg_len, x, y);
    }else if(filename.equals("wtr_fine_jpg")){
        M5.Lcd.drawJpg(wtr_fine_jpg, wtr_fine_jpg_len, x, y);
    }else if(filename.equals("wtr_rain_jpg")){
        M5.Lcd.drawJpg(wtr_rain_jpg, wtr_rain_jpg_len, x, y);
    }else if(filename.equals("wtr_snow_jpg")){
        M5.Lcd.drawJpg(wtr_snow_jpg, wtr_snow_jpg_len, x, y);
    }else if(filename.equals("wtr_uknw_jpg")){
        M5.Lcd.drawJpg(wtr_uknw_jpg, wtr_uknw_jpg_len, x, y);
    }
}

void drawJpgHeadFile(String filename,int x){
    int y=0;
    drawJpgHeadFile(filename,x, y);
}

void drawJpgHeadFile(String filename){
    int x=0,y=0;
    drawJpgHeadFile(filename,x, y);
}
