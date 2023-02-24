#ifndef wtr_uknw_jpg_len
    #include "jpgs/wtr_clud_jpg.h"
    #include "jpgs/wtr_fine_jpg.h"
    #include "jpgs/wtr_rain_jpg.h"
    #include "jpgs/wtr_snow_jpg.h"
    #include "jpgs/wtr_uknw_jpg.h"
#endif
#include "jpgs/wtr_clud_0_jpg.h"
#include "jpgs/wtr_fine_0_jpg.h"
#include "jpgs/wtr_rain_0_jpg.h"
#include "jpgs/wtr_snow_0_jpg.h"
#include "jpgs/wtr_uknw_0_jpg.h"
#include "jpgs/wtr_clud_1_jpg.h"
#include "jpgs/wtr_fine_1_jpg.h"
#include "jpgs/wtr_rain_1_jpg.h"
#include "jpgs/wtr_snow_1_jpg.h"
#include "jpgs/wtr_uknw_1_jpg.h"

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
    }else if(filename.equals("wtr_clud_0_jpg")){
        M5.Lcd.drawJpg(wtr_clud_0_jpg, wtr_clud_0_jpg_len, x, y);
    }else if(filename.equals("wtr_fine_0_jpg")){
        M5.Lcd.drawJpg(wtr_fine_0_jpg, wtr_fine_0_jpg_len, x, y);
    }else if(filename.equals("wtr_rain_0_jpg")){
        M5.Lcd.drawJpg(wtr_rain_0_jpg, wtr_rain_0_jpg_len, x, y);
    }else if(filename.equals("wtr_snow_0_jpg")){
        M5.Lcd.drawJpg(wtr_snow_0_jpg, wtr_snow_0_jpg_len, x, y);
    }else if(filename.equals("wtr_uknw_0_jpg")){
        M5.Lcd.drawJpg(wtr_uknw_0_jpg, wtr_uknw_0_jpg_len, x, y);
    }else if(filename.equals("wtr_clud_1_jpg")){
        M5.Lcd.drawJpg(wtr_clud_1_jpg, wtr_clud_1_jpg_len, x, y);
    }else if(filename.equals("wtr_fine_1_jpg")){
        M5.Lcd.drawJpg(wtr_fine_1_jpg, wtr_fine_1_jpg_len, x, y);
    }else if(filename.equals("wtr_rain_1_jpg")){
        M5.Lcd.drawJpg(wtr_rain_1_jpg, wtr_rain_1_jpg_len, x, y);
    }else if(filename.equals("wtr_snow_1_jpg")){
        M5.Lcd.drawJpg(wtr_snow_1_jpg, wtr_snow_1_jpg_len, x, y);
    }else if(filename.equals("wtr_uknw_1_jpg")){
        M5.Lcd.drawJpg(wtr_uknw_1_jpg, wtr_uknw_1_jpg_len, x, y);
    }
}

void drawJpgHeadFile(String filename,int x, int y, bool day){
    if(filename.substring(0,4).equals("wtr_")){
        if(!day){
            filename = filename.substring(0,9) + "0_jpg";
        }else{
            filename = filename.substring(0,9) + "1_jpg";
        }
    }
    drawJpgHeadFile(filename, x, y);
}

void drawJpgHeadFile(String filename,int x){
    int y=0;
    drawJpgHeadFile(filename,x, y);
}

void drawJpgHeadFile(String filename){
    int x=0,y=0;
    drawJpgHeadFile(filename,x, y);
}

void drawJpsHeadFilesTest(){
    for(int i=0; i<10;i++){
        drawJpgHeadFile(getWtrFileName(i%5),(i%4)*80,(i/4)*80,i/5);
    }
}

int getJpgHeadSize(String filename){
    if(filename.equals("wtr_clud_jpg")){
        return wtr_clud_jpg_len;
    }else if(filename.equals("wtr_fine_jpg")){
        return wtr_fine_jpg_len;
    }else if(filename.equals("wtr_rain_jpg")){
        return wtr_rain_jpg_len;
    }else if(filename.equals("wtr_snow_jpg")){
        return wtr_snow_jpg_len;
    }else if(filename.equals("wtr_uknw_jpg")){
        return wtr_uknw_jpg_len;
    }
    return 0;
}

String getWtrFileName(int i){
    const char wtrFiles[5][13]={
        "wtr_uknw_jpg", 
        "wtr_fine_jpg", 
        "wtr_clud_jpg", 
        "wtr_rain_jpg", 
        "wtr_snow_jpg"
    };
    if(i<0 || i>4) return "";
    return(String(wtrFiles[i]));
}

