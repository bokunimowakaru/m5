/***********************************************************************
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

#include <M5Stack.h>
#define TFT_GREY 0x5AEB                 //  0101 1010 1110 1011
                                        //  01011 010111 01011
#define TFT_DARK 0x18E3                 //  0001 1000 1110 0011
                                        //  00011 000111 00011

// uint32_t targetTime = 0;             // for next 1 second timeout
static uint8_t conv2d(const char* p);   // Forward declaration needed for IDE 1.6.x
uint8_t hh = conv2d(__TIME__), mm = conv2d(__TIME__ + 3),
        ss = conv2d(__TIME__ + 6);  // Get H, M, S from compile time

float degree_prev[3];
uint16_t buf_bg[40000];

void redrawNeedleLine(int i, int len, uint16_t color){
    uint16_t x, y;
    for(int r = 3; r < len; r++){
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

void clock_init(void) {
    float sx, sy;
    uint16_t x0, yy0, xn, yn;
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_DARK);
    M5.Lcd.fillCircle(160, 120, 119, TFT_DARK);     // Draw clock face
    M5.Lcd.fillCircle(160, 120, 118, DARKGREY);
    M5.Lcd.fillCircle(160, 120, 117, TFT_GREY);
    M5.Lcd.fillCircle(160, 120, 102, DARKGREY);
    M5.Lcd.fillCircle(160, 120, 101, TFT_DARK);
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
    
    M5.Lcd.fillCircle(160, 120, 3, TFT_RED);
    //M5.Lcd.readRect(60, 20, 200, 200, (uint16_t *)buf_bg);
    for(int i=0;i<40000;i++) buf_bg[i] = M5.Lcd.readPixel(i%200+60,i/200+20);
}

void clock_showText(String S){
    clearNeedleLine(0, 66);
    clearNeedleLine(1, 98);
    clearNeedleLine(2, 96);
    M5.Lcd.drawCentreString(S,160,150,2);
    for(int i=0;i<40000;i++) buf_bg[i] = M5.Lcd.readPixel(i%200+60,i/200+20);
    redrawNeedleLine(0, 66, TFT_WHITE);
    redrawNeedleLine(1, 98, TFT_WHITE);
    redrawNeedleLine(2, 96, TFT_RED);
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
        (mdeg - 18 < hdeg && hdeg < mdeg + 18)
      ||(sdeg - 18 < hdeg && hdeg < sdeg + 18)
      ||(int(hdeg - degree_prev[0] + 0.5))
    ) drawNeedleLine(0, hdeg, 66, TFT_WHITE);
    if(
        (sdeg - 18 < mdeg && mdeg < sdeg + 18)
      ||(int(mdeg - degree_prev[1] + 0.5))
    ) drawNeedleLine(1, mdeg, 98, TFT_WHITE);
    if(
         int(sdeg - degree_prev[2] + 0.5)
    ) drawNeedleLine(2, sdeg, 96, TFT_RED);
    M5.Lcd.fillCircle(160, 120, 3, TFT_RED);
}

void clock_Needle(){
    clock_Needle(0);
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
