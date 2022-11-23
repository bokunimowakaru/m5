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

#include "jpgs/clock_jpg.h"             // JPEG画像ファイル(木枠)を読み込む
#include "jpgs/clock2_jpg.h"            // JPEG画像ファイル(白木)を読み込む
#include "jpgs/wallpaper_jpg.h"         // JPEG画像ファイル(壁紙)を読み込む
#define TFT_GREY 0x5AEB                 //  01011 010111 01011
#define TFT_DARK 0x18E3                 //  00011 000111 00011

#define Faces_num 5                     // 時計盤の種類数
byte Face = 2;                          // 時計盤の種類の選択(0～2)

const uint16_t NeedleColor[Faces_num][4] = {  // 針の色設定 短針,長針,秒針
    {TFT_GREEN,TFT_WHITE,TFT_WHITE,TFT_RED},
    {TFT_GREEN,TFT_BLACK,TFT_BLACK,TFT_RED},
    {TFT_GREEN,TFT_WHITE,TFT_WHITE,TFT_RED},
    {TFT_GREEN,TFT_BLACK,TFT_BLACK,TFT_RED},
    {TFT_GREEN,TFT_BLACK,TFT_BLACK,TFT_RED}
};
const byte NeedleLen[Faces_num][4] = {        // 短針(時針)の長さ設定
    {33,66,98,96},
    {33,66,98,96},
    {30,60,89,86},
    {30,54,89,86},
    {33,66,98,96}
};
const uint16_t TextColor[Faces_num] = {       // 時計盤の文字色
    TFT_WHITE, TFT_BLACK, TFT_WHITE, TFT_BLACK, TFT_BLACK
};
const uint16_t BgColor[Faces_num] = {         // 時計盤の背景色
    TFT_DARK, LIGHTGREY, TFT_BLACK, TFT_WHITE, TFT_WHITE
};

// uint32_t targetTime = 0;             // for next 1 second timeout
static uint8_t conv2d(const char* p);   // Forward declaration needed for IDE 1.6.x
uint8_t hh = conv2d(__TIME__), mm = conv2d(__TIME__ + 3),
        ss = conv2d(__TIME__ + 6);  // Get H, M, S from compile time

float degree_prev[4];
float degree_alrm = 0;
boolean AlarmEn = false;

uint16_t *buf_bg = NULL;
String TEXT[2] = {"",""};

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
    for(int i=AlarmEn?0:1; i<4; i++) redrawNeedleLine(i, NeedleLen[Face][i],NeedleColor[Face][i]);
    M5.Lcd.fillCircle(160, 120, 3, TFT_RED);
}

int clock_init(int clockface) {
    if(0 <= clockface && clockface < Faces_num) Face = clockface; else Face = 0;

    if(!buf_bg){
        if(psramInit()){
            Serial.println("installed PSRAM = " + String(ESP.getPsramSize()/1024) + "K Bytes");
            buf_bg = (uint16_t *) ps_malloc(80000);     // 200 * 200 * 2 Bytes (80 K Bytes)
        }else{
            Serial.println("NO PSRAM, so buffer buf_bg placed in nomal malloc.");
            buf_bg = (uint16_t *) malloc(80000);        // 200 * 200 * 2 Bytes
        }
    }
    float sx, sy;
    uint16_t x0, yy0, xn, yn;
    M5.Lcd.fillScreen(TFT_BLACK);

    switch(Face){
        case 4:
            M5.Lcd.drawJpg(wallpaper_jpg, wallpaper_jpg_len);
            M5.Lcd.setTextColor(TextColor[Face]);
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
            M5.Lcd.setTextColor(TextColor[Face]);
            break;
        case 2:
            M5.Lcd.drawJpg(clock_jpg, clock_jpg_len);
            M5.Lcd.setTextColor(TextColor[Face]);
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
            M5.Lcd.fillCircle(160, 120, 101, BgColor[Face]);
            M5.Lcd.setTextColor(TextColor[Face]);
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

    M5.Lcd.fillCircle(160, 120, 3, TFT_RED);
    //M5.Lcd.readRect(60, 20, 200, 200, (uint16_t *)buf_bg);
    for(int i=0;i<40000;i++) buf_bg[i] = M5.Lcd.readPixel(i%200+60,i/200+20);
    clock_redrawNeedle();
    for(int i=0;i<2;i++) TEXT[i] = ""; 
    return Face;
}

int clock_init(){
    return clock_init(Face);
}

void clock_showText(String S, int pos){
    if(pos > 100) pos = 100;    // 上部表示 +46あたり にする
    if(pos < -84) pos = -84;    // 下部表示 -30あたり にする
    if(S.length() > 10) S = S.substring(0,10); // 10文字以内

    int pos2 = pos < 0 ? 0 : 1;
    for(int i=0; i<4; i++) clearNeedleLine(i, NeedleLen[Face][i]);
    if(TEXT[pos2] != "" && TEXT[pos2] != S){
        M5.Lcd.fillRect(120,120-pos,80,15,BgColor[Face]);
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
    uint8_t hour = (ms / 3600000) % 24;
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
    ) drawNeedleLine(1, hdeg, NeedleLen[Face][1],NeedleColor[Face][1]);
    if(
        (fmod(fabs(sdeg - mdeg),360) < 16)
      ||(int(mdeg - degree_prev[2] + 0.5))
    ) drawNeedleLine(2, mdeg, NeedleLen[Face][2],NeedleColor[Face][2]);
    if(AlarmEn && (
        (fmod(fabs(sdeg - degree_alrm),360) < 16)
    )) drawNeedleLine(0, degree_alrm, NeedleLen[Face][0],NeedleColor[Face][0]);
    if(
         int(sdeg - degree_prev[3] + 0.5)
    ) drawNeedleLine(3, sdeg, NeedleLen[Face][3],NeedleColor[Face][3]);
    M5.Lcd.fillCircle(160, 120, 3, TFT_RED);
}

void clock_Needle(){
    clock_Needle(0);
}

void clock_AlarmOff(){
    if(AlarmEn) clearNeedleLine(0, NeedleLen[Face][0]);
    AlarmEn = false;
    clock_redrawNeedle();
}

void clock_Alarm(unsigned long ms){
    if(ms != 43200000ul){
        ms %= 43200000ul;
        AlarmEn = true;
        degree_alrm = (float)ms / 3600000. * 30.;
        drawNeedleLine(0, degree_alrm, NeedleLen[Face][0],NeedleColor[Face][0]);
    }else{
        clock_AlarmOff();
    }
}


static uint8_t conv2d(const char* p) {
    uint8_t v = 0;
    if ('0' <= *p && *p <= '9') v = *p - '0';
    return 10 * v + *++p - '0';
}

/*******************************************************************************

// Saved H, M, S x & y coords
uint16_t osx = 160, osy = 120, omx = 160, omy = 120, ohx = 160, ohy = 120;
// uint32_t targetTime = 0;             // for next 1 second timeout
static uint8_t conv2d(const char* p);   // Forward declaration needed for IDE 1.6.x
uint8_t hh = conv2d(__TIME__), mm = conv2d(__TIME__ + 3),
        ss = conv2d(__TIME__ + 6);  // Get H, M, S from compile time
boolean reDrawNeedle = 1;

void clock_init(void) {
    // M5.begin();
    // M5.Power.begin();
    // M5.Lcd.setRotation(0);
    // M5.Lcd.fillScreen(TFT_BLACK);
    // M5.Lcd.fillScreen(TFT_RED);
    // M5.Lcd.fillScreen(TFT_GREEN);
    // M5.Lcd.fillScreen(TFT_BLUE);
    // M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.fillScreen(TFT_BLACK);
    // Adding a background colour erases previous text automatically
    M5.Lcd.setTextColor(TFT_WHITE, TFT_DARK);

    // Draw clock face
    M5.Lcd.fillCircle(160, 120, 119, TFT_DARK);
    M5.Lcd.fillCircle(160, 120, 118, DARKGREY);
    M5.Lcd.fillCircle(160, 120, 117, TFT_GREY);
    M5.Lcd.fillCircle(160, 120, 102, DARKGREY);
    M5.Lcd.fillCircle(160, 120, 101, TFT_DARK);

    float sx, sy;        // float mx = 1, my = 0, hx = -1, hy = 0;
    uint16_t x0, yy0;    // uint16_t x1 = 0, yy1 = 0;
    // Draw 12 lines
    //  for (int i = 0; i < 360; i += 30) {
    //      sx  = cos((i - 90) * 0.0174532925);
    //      sy  = sin((i - 90) * 0.0174532925);
    //      x0  = sx * 114 + 160;
    //      yy0 = sy * 114 + 120;
    //      x1  = sx * 100 + 160;
    //      yy1 = sy * 100 + 120;
    //      M5.Lcd.drawLine(x0, yy0, x1, yy1, TFT_GREEN);
    //  }

    // Draw 60 dots
    for (int i = 0; i < 360; i += 6) {
        sx  = cos((i - 90) * 0.0174532925);
        sy  = sin((i - 90) * 0.0174532925);
        //  x0  = sx * 102 + 160;
        //  yy0 = sy * 102 + 120;
        //  // Draw minute markers
        //  M5.Lcd.drawPixel(x0, yy0, TFT_WHITE);
        //  // Draw main quadrant dots
        //  if (i == 0 || i == 180) M5.Lcd.fillCircle(x0, yy0, 2, TFT_WHITE);
        //  if (i == 90 || i == 270) M5.Lcd.fillCircle(x0, yy0, 2, TFT_WHITE);
        x0  = sx * 109 + 160;
        yy0 = sy * 109 + 120;
        if(i % 90 == 0){
            M5.Lcd.fillCircle(x0, yy0, 5, DARKGREY);
            M5.Lcd.fillCircle(x0, yy0, 4, TFT_WHITE);
        }else if(i % 5 == 0){
            M5.Lcd.fillCircle(x0, yy0, 3, DARKGREY);
            M5.Lcd.fillCircle(x0, yy0, 2, LIGHTGREY);
        }else {
            M5.Lcd.fillCircle(x0, yy0, 1, TFT_DARK);
        }
    }
    // M5.Lcd.fillCircle(160, 120, 3, TFT_WHITE);
    // M5.Lcd.drawCentreString("M5Stack", 120, 260, 4);
    // targetTime = millis() + 1000;
}

void clock_Needle(unsigned long ms){
    if(!ms) ms = (((hh * 60 + mm) * 60 + ss) * 1000) + millis();
    uint8_t hour = (ms / 3600000) % 24;
    uint8_t min  = (ms / 60000) % 60;
    uint8_t sec  = (ms / 1000) % 60;
    uint8_t centi  = (ms/10) % 100;

    // Saved H, M, S x & y multipliers
    float sx = 0, sy = 1, mx = 1, my = 0, hx = -1, hy = 0;
    float sdeg = 0., mdeg = 0., hdeg = 0.;

    // Pre-compute hand degrees, x & y coords for a fast screen update
    // sdeg = (float)sec * 6.;                       // 0-59 -> 0-354
    sdeg = ((float)sec + (float)centi / 100.) * 6.;
    mdeg = ((float)min + (float)sec / 60.) * 6.;  // 0-59 -> 0-360 - includes seconds
    hdeg = (float)hour * 30. + mdeg * 0.0833333;  // 0-11 -> 0-360 - includes minutes and seconds

    hx = cos((hdeg - 90) * 0.0174532925);
    hy = sin((hdeg - 90) * 0.0174532925);
    mx = cos((mdeg - 90) * 0.0174532925);
    my = sin((mdeg - 90) * 0.0174532925);
    sx = cos((sdeg - 90) * 0.0174532925);
    sy = sin((sdeg - 90) * 0.0174532925);

    if (sec%2 != 0) reDrawNeedle = 1;
    else if (reDrawNeedle) {
        reDrawNeedle = 0;
        // Erase hour and minute hand positions
        M5.Lcd.drawLine(ohx, ohy, 160, 120, TFT_DARK);
        M5.Lcd.drawLine(omx, omy, 160, 120, TFT_DARK);
        ohx  =  hx * 66 + 160;
        ohy  =  hy * 66 + 120;
        omx  =  mx * 98 + 160;
        omy  =  my * 98 + 120;
    }

    // Redraw new hand positions, hour and minute hands not erased here to
    // avoid flicker
    M5.Lcd.drawLine(osx, osy, 160, 120, TFT_DARK);
    osx  = sx * 96 + 160;
    osy  = sy * 96 + 120;
    M5.Lcd.drawLine(160, 120, osx, osy, TFT_RED);
    M5.Lcd.drawLine(ohx, ohy, 160, 120, TFT_WHITE);
    M5.Lcd.fillCircle(160, 120, 3, TFT_RED);
    M5.Lcd.drawLine(omx, omy, 160, 120, TFT_WHITE);
    M5.Lcd.fillCircle(160, 120, 3, TFT_RED);
    M5.Lcd.drawLine(osx, osy, 160, 120, TFT_RED);
    M5.Lcd.fillCircle(160, 120, 3, TFT_RED);
}
*******************************************************************************/
