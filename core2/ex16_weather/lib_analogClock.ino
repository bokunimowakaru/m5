/*******************************************************************************
analogClock

本ソースコードは下記からダウンロードしたものを元に改変しました。
https://github.com/m5stack/M5Stack/blob/master/examples/Advanced/Display/TFT_Clock/TFT_Clock.ino

改変部の著作権は当方が保有します。無保証です。

                                          Copyright (c) 2022 Wataru KUNINO

元のソースコードのライセンスについては下記を参照してください。
https://github.com/m5stack/M5Stack/blob/master/LICENSE

m5stack/M5Stack is licensed under the MIT License

2022/11/14

以下、TFT_Clock 内の表示(一部)

/*
 An example analogue clock using a TFT LCD screen to show the time
 use of some of the drawing commands with the library.

 For a more accurate clock, it would be better to use the RTClib library.
 But this is just a demo.

 Based on a sketch by Gilchrist 6/2/2014 1.0
 */

#ifndef wtr_uknw_jpg_len
    #include "jpgs/wtr_clud_jpg.h"
    #include "jpgs/wtr_fine_jpg.h"
    #include "jpgs/wtr_rain_jpg.h"
    #include "jpgs/wtr_snow_jpg.h"
    #include "jpgs/wtr_uknw_jpg.h"
#endif

#define TFT_GREY 0x5AEB                 //  01011 010111 01011
#define TFT_DARK 0x18E3                 //  00011 000111 00011

#define wtrFaces_num 7                     // 時計盤の種類数
byte wtrFace = 5;                       // 時計盤の種類の選択(0～2)
byte wtrHour = 0;                       // 24時間制の現在時 0～23

const uint16_t NeedleColor[wtrFaces_num][6] = {  // 針の色設定 短針,長針,秒針
    {TFT_GREEN,TFT_WHITE,TFT_WHITE,TFT_RED},
    {TFT_GREEN,TFT_BLACK,TFT_BLACK,TFT_RED},
    {TFT_GREEN,TFT_WHITE,TFT_WHITE,TFT_RED},
    {TFT_GREEN,TFT_BLACK,TFT_BLACK,TFT_RED},
    {TFT_GREEN,TFT_BLACK,TFT_BLACK,TFT_RED},
    {TFT_GREEN,TFT_BLACK,TFT_BLACK,TFT_RED},
    {TFT_GREEN,TFT_WHITE,TFT_WHITE,TFT_RED}
};
const byte NeedleLen[wtrFaces_num][7] = {        // 短針(時針)の長さ設定
    {33,66,98,96},
    {33,66,98,96},
    {30,60,89,86},
    {30,54,89,86},
    {33,66,98,96},
    {33,66,98,96},
    {33,66,98,96}
};
const uint16_t TextColor[wtrFaces_num] = {       // 時計盤の文字色
    TFT_WHITE, TFT_BLACK, TFT_WHITE, TFT_BLACK, TFT_BLACK, TFT_BLACK, TFT_WHITE
};
const uint16_t BgColor[wtrFaces_num] = {         // 時計盤の背景色
    TFT_DARK, LIGHTGREY, TFT_BLACK, TFT_WHITE, TFT_WHITE, TFT_WHITE, TFT_BLACK
};

// uint32_t targetTime = 0;             // for next 1 second timeout
static uint8_t conv2d(const char* p);   // Forward declaration needed for IDE 1.6.x
uint8_t hh = conv2d(__TIME__), mm = conv2d(__TIME__ + 3),
        ss = conv2d(__TIME__ + 6);  // Get H, M, S from compile time

float degree_prev[4];
float degree_alrm = 0;
boolean AlarmEn = false;

uint16_t *buf_bg = NULL;
uint16_t *buf_txt = NULL;
String TEXT[2] = {"",""};
char wtrFileName[13] = "";
int wtrFileLength = 0;
int wtrCodeInADay[2] = {0,0};
int wtrTemp[4][2];   // 時刻;気温temps
byte wtrPop[6][2];           // 時刻;降水確率pops

void redrawNeedleLine(int i, int len, uint16_t color){
    uint16_t x, y;
    for(int r = 3 ; r < len; r++){
        x  = r * cos((degree_prev[i] - 90) * 0.0174532925) + 160;
        y  = r * sin((degree_prev[i] - 90) * 0.0174532925) + 120;
        M5.Lcd.drawPixel(x, y, color);
    }
}

void clearNeedleLine(int i, int len){
    uint16_t x, y;
    for(int r = 3; r < len; r++){
        x  = r * cos((degree_prev[i] - 90) * 0.0174532925) + 160;
        y  = r * sin((degree_prev[i] - 90) * 0.0174532925) + 120;
        if(x < 60) x = 60;
        if(x > 259) x = 259;
        if(y < 20) y = 20;
        if(y > 219) y = 219;
        M5.Lcd.drawPixel(x, y, buf_bg[(y-20)*200+(x-60)]);
    }
}

void drawNeedleLine(int i, float degree, int len, uint16_t color){
    uint16_t x, y;
    clearNeedleLine(i, len);
    for(int r = 3; r < len; r++){
        x  = r * cos((degree - 90) * 0.0174532925) + 160;
        y  = r * sin((degree - 90) * 0.0174532925) + 120;
        M5.Lcd.drawPixel(x, y, color);
    }
    degree_prev[i] = degree;
}

void clock_redrawNeedle(){
    for(int i=AlarmEn?0:1; i<4; i++) redrawNeedleLine(i, NeedleLen[wtrFace][i],NeedleColor[wtrFace][i]);
    M5.Lcd.fillCircle(160, 120, 3, TFT_RED);
}

int clock_init(int clockface) {
    if(0 <= clockface && clockface < wtrFaces_num) wtrFace = clockface; else wtrFace = 0;
    if(wtrFace <= 3) wtrFace = 4;  // 天気専用
    Serial.printf("DEBUG clock_init wtrFace = %d\n", wtrFace);

    if(!buf_bg){
        if(psramInit()){
            Serial.println("installed PSRAM = " + String(ESP.getPsramSize()/1024) + "K Bytes");
            buf_bg = (uint16_t *) ps_malloc(80000+7680);  // 200 * 200 * 2 Bytes (80 k Bytes)
            buf_txt = buf_bg + 80000;                     // 80 * 16 * 2 * 2 Bytes (5120 Bytes)
        }else{
            Serial.println("NO PSRAM, so buffer buf_bg placed in nomal malloc.");
            buf_bg = (uint16_t *) malloc(80000);        // 200 * 200 * 2 Bytes
        }
    }
    float sx, sy;
    uint16_t x0, yy0, xn, yn;
    M5.Lcd.fillScreen(TFT_BLACK);
/*
    switch(wtrFace){
        case 4:
            M5.Lcd.drawJpg(wallpaper_jpg, wallpaper_jpg_len);
            M5.Lcd.setTextColor(TextColor[wtrFace]);
            for(int i = 0; i < 360; i += 30){
                sx  = cos((i - 90) * 0.0174532925);
                sy  = sin((i - 90) * 0.0174532925);
                x0  = sx * 90 + 160;
                yy0 = sy * 90 + 120;
                xn  = sx * 109 + 160;
                yn  = sy * 109 + 120;
                M5.Lcd.fillCircle(x0, yy0, 3, DARKGREY);
                M5.Lcd.drawCentreString(String(i ? i/30 : 12),xn,yn-10,4);
            }
            break;
        case 3:
            M5.Lcd.drawJpg(clock2_jpg, clock_jpg_len);
            M5.Lcd.setTextColor(TextColor[wtrFace]);
            break;
        case 2:
            M5.Lcd.drawJpg(clock_jpg, clock_jpg_len);
            M5.Lcd.setTextColor(TextColor[wtrFace]);
            for(int i = 0; i < 360; i += 30){
                xn = cos((i - 90) * 0.0174532925) * 70 + 160;
                yn = sin((i - 90) * 0.0174532925) * 70 + 120;
                if(i % 90 == 0){
                      M5.Lcd.drawCentreString(String(i ? i/30 : 12),xn,yn-10,4);
                }else M5.Lcd.drawCentreString(String(i/30),xn,yn-8,2);
            }
            break;
        default:
            M5.Lcd.fillCircle(160, 120, 119, TFT_DARK);     // Draw clock face
            M5.Lcd.fillCircle(160, 120, 118, DARKGREY);
            M5.Lcd.fillCircle(160, 120, 117, TFT_GREY);
            M5.Lcd.fillCircle(160, 120, 102, DARKGREY);
            M5.Lcd.fillCircle(160, 120, 101, BgColor[wtrFace]);
            M5.Lcd.setTextColor(TextColor[wtrFace]);
            for (int i = 0; i < 360; i += 6) {              // Draw 60 dots
                sx  = cos((i - 90) * 0.0174532925);
                sy  = sin((i - 90) * 0.0174532925);
                x0  = sx * 109 + 160;
                yy0 = sy * 109 + 120;
                xn = sx * 86 + 160;
                yn = sy * 86 + 120;
                if(i % 90 == 0){                            // 12時,3時,6時,9時
                    M5.Lcd.fillCircle(x0, yy0, 5, DARKGREY);
                    M5.Lcd.fillCircle(x0, yy0, 4, TFT_WHITE);
                    M5.Lcd.drawCentreString(String(i ? i/30 : 12),xn,yn-10,4);
                }else if(i % 5 == 0){                       // 1,2,4,5,7,8,10,11時
                    M5.Lcd.fillCircle(x0, yy0, 3, DARKGREY);
                    M5.Lcd.fillCircle(x0, yy0, 2, LIGHTGREY);
                    M5.Lcd.drawCentreString(String(i/30),xn,yn-8,2);
                }else {
                    M5.Lcd.fillCircle(x0, yy0, 1, TFT_DARK);
                }
            }
            break;
    }
*/    
    if(wtrFace == 4){
        wtrFileLength = getJpgHeadSize(wtrFileName);
        if(wtrFileLength == 0 || strlen(wtrFileName) == 0 ){
            Serial.printf("DEBUG clock_init wtrFace = %d wtrFileLength=%d wtrFileName=%s\n", wtrFace, wtrFileLength, wtrFileName);
            M5.Lcd.drawJpg(wtr_uknw_jpg, wtr_uknw_jpg_len);
        }else{
            Serial.printf("DEBUG clock_init wtrFace = %d wtrFileName=%s\n", wtrFace, wtrFileName);
            drawJpgHeadFile(wtrFileName);
        }
    }else if(wtrFace == 5 || wtrFace == 6){
        for(int x=0; x<320; x+=80)for(int y=0; y<240; y+=80){
            drawJpgHeadFile(getWtrFileName(0),x,y,wtrFace==5?1:0);
        }
    }

    M5.Lcd.setTextColor(TextColor[wtrFace]);
    for(int i = 0; i < 360; i += 30){
        sx  = cos((i - 90) * 0.0174532925);
        sy  = sin((i - 90) * 0.0174532925);
        x0  = sx * 88 + 160;
        yy0 = sy * 88 + 120;
        xn  = sx * 109 + 160;
        yn  = sy * 109 + 120;
        M5.Lcd.fillCircle(x0, yy0, 3, DARKGREY);
        // M5.Lcd.drawCentreString(String(i ? i/30 : 12),xn,yn-10,4);
    }

    M5.Lcd.fillCircle(160, 120, 3, TFT_RED);
    //M5.Lcd.readRect(60, 20, 200, 200, (uint16_t *)buf_bg);
    for(int i=0;i<40000;i++) buf_bg[i] = M5.Lcd.readPixel(i%200+60,i/200+20);
    if(buf_txt){
        for(int p=0; p<2; p++){
            for(int x=0; x<80; x++){
                for(int y=0; y<16; y++){
                    buf_txt[x+y*80+p*2560] = M5.Lcd.readPixel(x+120,y+120-(p==0?-30:46));
                }
            }
        }
    }
    clock_redrawNeedle();
    for(int i=0;i<2;i++) TEXT[i] = ""; 
    return wtrFace;
}

int clock_init(int clockface, const char *filename) {
    strncpy(wtrFileName,filename,13);
    wtrFileLength = getJpgHeadSize(filename);
    return clock_init(clockface);
}


int clock_init(){
    return clock_init(wtrFace);
}

void clock_showWeather(){
    if(wtrFace != 5 && wtrFace != 6) return;
    drawJpgHeadFile(getWtrFileName(wtrCodeInADay[0]+1),0,0,wtrFace==5?1:0);
    drawJpgHeadFile(getWtrFileName(wtrCodeInADay[1]+1),240,0,wtrFace==5?1:0);
    if( wtrHour >= 12 && wtrHour < 18 ){
        M5.Lcd.drawCentreString("Night",40,80-8*(7-wtrFace),1);
    }else if( wtrHour>= 18 ){
        M5.Lcd.drawCentreString("Tomorrow",280,80-8*(7-wtrFace),1);
    }
}

void clock_showWeather(int c0,int c1){
    wtrCodeInADay[0]=c0;
    wtrCodeInADay[1]=c1;
    strcpy(wtrFileName, wtrFiles[wtrCodeInADay[0]+1]);
    wtrFileLength = getJpgHeadSize(wtrFileName);
    if(wtrFace == 5 || wtrFace == 6) clock_showWeather();
}

void clock_showTemperature(){
    if(wtrFace != 5 && wtrFace != 6) return;
    M5.Lcd.drawCentreString("Temp.",50,180,2);
    String S = "-";
    int day_prev = 0;
    for(int i=0; i<4; i++){
        if(i>0 && day_prev != wtrTemp[i][0] /24){
            M5.Lcd.drawCentreString("|",20*i, 200, 2);
            M5.Lcd.drawCentreString("|",20*i, 220, 2);
        }
        day_prev = wtrTemp[i][0]/24;
        if(wtrTemp[i][0] >= 0) S = String(wtrTemp[i][0] % 24); else S = "-";
        M5.Lcd.drawCentreString(S, 20*i+10, 200, 2);
        if(wtrTemp[i][1] >= -100) S = String(wtrTemp[i][1]); else S = "-";
        M5.Lcd.drawCentreString(S, 20*i+10, 220, 2);
    }
    M5.Lcd.drawCentreString("h",90,200,2);
    M5.Lcd.drawCentreString("C",90,220,2);
}

void clock_showTemperature(int temp_s[][2]){
    for(int i=0; i<4; i++){
        wtrTemp[i][0] = temp_s[i][0];
        wtrTemp[i][1] = temp_s[i][1];
    }
    clock_showTemperature();
}

void clock_showPop(){
    if(wtrFace != 5 && wtrFace != 6) return;
    M5.Lcd.drawCentreString("P.O.P.",265,180,2);
    String S = "-";
    for(int i=0; i<4; i++){
        if(wtrPop[i+1][0] >= 0) S = String(wtrPop[i+1][0]); else S = "-";
        M5.Lcd.drawCentreString(S,20*i+230,200,2);
        if(wtrPop[i+1][1] >= 0) S = String(wtrPop[i+1][1]); else S = "-";
        M5.Lcd.drawCentreString(S,20*i+230,220,2);
    }
    M5.Lcd.drawCentreString("h",310,200,2);
    M5.Lcd.drawCentreString("%",310,220,2);
}

void clock_showPop(byte pop_s[][2]){
    for(int i=0; i<6; i++){
        wtrPop[i][0] = pop_s[i][0];
        wtrPop[i][1] = pop_s[i][1];
    }
    clock_showPop();
}

void clock_showText(String S, int pos){
    if(pos > 0) pos = 46;    // 上部表示 +46あたり にする
    if(pos <= 0) pos = -30;    // 下部表示 -30あたり にする
    if(S.length() > 12) S = S.substring(0,12); // 12文字以内

    int pos2 = pos < 0 ? 0 : 1;
    for(int i=0; i<4; i++) clearNeedleLine(i, NeedleLen[wtrFace][i]);
    if(TEXT[pos2] != "" && TEXT[pos2] != S){
        if(buf_txt){
            for(int x=0; x<80; x++){
                for(int y=0; y<16; y++){
                    M5.Lcd.drawPixel(x+120, y+120-pos, buf_txt[x+y*80+pos2*2560 ]);
                }
            }
        }else{
            M5.Lcd.fillRect(120,120-pos,80,15,BgColor[wtrFace]);
        }
    }
    M5.Lcd.drawCentreString(S,160,120-pos,2);
    for(int x=0; x<120; x++){
        for(int y=0; y<16; y++){
            buf_bg[x+40+(y+100-pos)*200] = M5.Lcd.readPixel(x+100,y+120-pos);
        }
    }
    clock_redrawNeedle();
    TEXT[pos2] = S;
}

void clock_showText(String S){
    clock_showText(S, -30);  // Possison = Bottom
}

void clock_Needle(unsigned long ms){
    if(!ms) ms = (((hh * 60 + mm) * 60 + ss) * 1000) + millis();
    wtrHour = (ms / 3600000) % 24;
    ms %= 43200000;                           // 12時間制に変換
    uint8_t hour = (ms / 3600000) % 12;
    uint8_t min  = (ms / 60000) % 60;
    uint8_t sec  = (ms / 1000) % 60;
    uint8_t centi  = (ms/10) % 100;
    float sdeg = ((float)sec + (float)centi / 100.) * 6.;
    float mdeg = ((float)min + (float)sec / 60.) * 6.;
    float hdeg = (float)hour * 30. + mdeg * 0.0833333;

    if(
        (fmod(fabs(mdeg - hdeg),360) < 16)
      ||(fmod(fabs(sdeg - hdeg),360) < 16)
      ||(int(hdeg - degree_prev[1] + 0.5))
    ) drawNeedleLine(1, hdeg, NeedleLen[wtrFace][1],NeedleColor[wtrFace][1]);
    if(
        (fmod(fabs(sdeg - mdeg),360) < 16)
      ||(int(mdeg - degree_prev[2] + 0.5))
    ) drawNeedleLine(2, mdeg, NeedleLen[wtrFace][2],NeedleColor[wtrFace][2]);
    if(AlarmEn && (
        (fmod(fabs(sdeg - degree_alrm),360) < 16)
    )) drawNeedleLine(0, degree_alrm, NeedleLen[wtrFace][0],NeedleColor[wtrFace][0]);
    if(
         int(sdeg - degree_prev[3] + 0.5)
    ) drawNeedleLine(3, sdeg, NeedleLen[wtrFace][3],NeedleColor[wtrFace][3]);
    M5.Lcd.fillCircle(160, 120, 3, TFT_RED);
}

void clock_Needle(){
    clock_Needle(0);
}

void clock_AlarmOff(){
    if(AlarmEn) clearNeedleLine(0, NeedleLen[wtrFace][0]);
    AlarmEn = false;
    clock_redrawNeedle();
}

void clock_Alarm(unsigned long ms){
    if(ms != 43200000ul){
        ms %= 43200000ul;
        AlarmEn = true;
        degree_alrm = (float)ms / 3600000. * 30.;
        drawNeedleLine(0, degree_alrm, NeedleLen[wtrFace][0],NeedleColor[wtrFace][0]);
    }else{
        clock_AlarmOff();
    }
}

static uint8_t conv2d(const char* p) {
    uint8_t v = 0;
    if ('0' <= *p && *p <= '9') v = *p - '0';
    return 10 * v + *++p - '0';
}
