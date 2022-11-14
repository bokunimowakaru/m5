/***********************************************************************
analogClock

本ソースコードは下記からダウンロードしたものです。
https://github.com/m5stack/M5Stack/blob/master/examples/Advanced/Display/TFT_Clock/TFT_Clock.ino

ライセンスについては下記を参照してください。
https://github.com/m5stack/M5Stack/blob/master/LICENSE

m5stack/M5Stack is licensed under the MIT License

2022/11/14

以下、TFT_Meter_linear

/*
 An example analogue clock using a TFT LCD screen to show the time
 use of some of the drawing commands with the library.

 For a more accurate clock, it would be better to use the RTClib library.
 But this is just a demo.

 This sketch uses font 4 only.

 Make sure all the display driver and pin comnenctions are correct by
 editting the User_Setup.h file in the TFT_eSPI library folder.

 #########################################################################
 ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
 #########################################################################

 Based on a sketch by Gilchrist 6/2/2014 1.0
 */

#include <M5Stack.h>

#define TFT_GREY 0x5AEB

float sx = 0, sy = 1, mx = 1, my = 0, hx = -1,
      hy   = 0;  // Saved H, M, S x & y multipliers
float sdeg = 0., mdeg = 0., hdeg = 0.;
uint16_t osx = 160, osy = 120, omx = 160, omy = 120, ohx = 160,
         ohy = 120;  // Saved H, M, S x & y coords
uint16_t x0 = 0, x1 = 0, yy0 = 0, yy1 = 0;
uint32_t targetTime = 0;  // for next 1 second timeout

static uint8_t conv2d(
    const char* p);  // Forward declaration needed for IDE 1.6.x
/*
uint8_t hh = conv2d(__TIME__), mm = conv2d(__TIME__ + 3),
        ss = conv2d(__TIME__ + 6);  // Get H, M, S from compile time
*/

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

    M5.Lcd.setTextColor(TFT_WHITE,
                        TFT_BLACK);  // Adding a background colour erases
                                    // previous text automatically

    // Draw clock face
    M5.Lcd.fillCircle(160, 120, 118, TFT_GREY);
    M5.Lcd.fillCircle(160, 120, 110, TFT_BLACK);

    // Draw 12 lines
    for (int i = 0; i < 360; i += 30) {
        sx  = cos((i - 90) * 0.0174532925);
        sy  = sin((i - 90) * 0.0174532925);
        x0  = sx * 114 + 160;
        yy0 = sy * 114 + 120;
        x1  = sx * 100 + 160;
        yy1 = sy * 100 + 120;

        M5.Lcd.drawLine(x0, yy0, x1, yy1, TFT_GREEN);
    }

    // Draw 60 dots
    for (int i = 0; i < 360; i += 6) {
        sx  = cos((i - 90) * 0.0174532925);
        sy  = sin((i - 90) * 0.0174532925);
        x0  = sx * 102 + 160;
        yy0 = sy * 102 + 120;
        // Draw minute markers
        M5.Lcd.drawPixel(x0, yy0, TFT_WHITE);

        // Draw main quadrant dots
        if (i == 0 || i == 180) M5.Lcd.fillCircle(x0, yy0, 2, TFT_WHITE);
        if (i == 90 || i == 270) M5.Lcd.fillCircle(x0, yy0, 2, TFT_WHITE);
    }

    M5.Lcd.fillCircle(160, 121, 3, TFT_WHITE);

    // Draw text at position 120,260 using fonts 4
    // Only font numbers 2,4,6,7 are valid. Font 6 only contains characters
    // [space] 0 1 2 3 4 5 6 7 8 9 : . - a p m Font 7 is a 7 segment font and
    // only contains characters [space] 0 1 2 3 4 5 6 7 8 9 : .
    // M5.Lcd.drawCentreString("M5Stack", 120, 260, 4);

    // targetTime = millis() + 1000;
}

void clock_Needle(unsigned long ms){
	uint8_t hour = (ms / 3600000) % 24;
	uint8_t min  = (ms / 60000) % 60;
	uint8_t sec  = (ms / 1000) % 60;
	uint8_t centi  = (ms/10) % 100;
	
	/*
    // Pre-compute hand degrees, x & y coords for a fast screen update
    sdeg = (float)sec * 6.;                               // 0-59 -> 0-354
    */
    sdeg = ((float)sec + (float)centi / 100.) * 6.;
    mdeg = ((float)min + (float)sec / 60.) * 6.;  // 0-59 -> 0-360 - includes seconds
    hdeg =
        (float)hour * 30. +
        mdeg * 0.0833333;  // 0-11 -> 0-360 - includes minutes and seconds
    
    hx = cos((hdeg - 90) * 0.0174532925);
    hy = sin((hdeg - 90) * 0.0174532925);
    mx = cos((mdeg - 90) * 0.0174532925);
    my = sin((mdeg - 90) * 0.0174532925);
    sx = cos((sdeg - 90) * 0.0174532925);
    sy = sin((sdeg - 90) * 0.0174532925);

    if (sec%3 != 0) reDrawNeedle = 1;
    else if (reDrawNeedle) {
        reDrawNeedle = 0;
        // Erase hour and minute hand positions every minute
        M5.Lcd.drawLine(ohx, ohy, 160, 121, TFT_BLACK);
        ohx = hx * 62 + 160;
        ohy = hy * 62 + 121;
        M5.Lcd.drawLine(omx, omy, 160, 121, TFT_BLACK);
        omx = mx * 84 + 160;
        omy = my * 84 + 121;
    }

    // Redraw new hand positions, hour and minute hands not erased here to
    // avoid flicker
    M5.Lcd.drawLine(osx, osy, 160, 121, TFT_BLACK);
    osx = sx * 90 + 161;
    osy = sy * 90 + 121;
    M5.Lcd.drawLine(osx, osy, 160, 121, TFT_RED);
    M5.Lcd.drawLine(ohx, ohy, 160, 121, TFT_WHITE);
    M5.Lcd.drawLine(omx, omy, 160, 121, TFT_WHITE);
    M5.Lcd.drawLine(osx, osy, 160, 121, TFT_RED);

    M5.Lcd.fillCircle(160, 121, 3, TFT_RED);
}

static uint8_t conv2d(const char* p) {
    uint8_t v = 0;
    if ('0' <= *p && *p <= '9') v = *p - '0';
    return 10 * v + *++p - '0';
}
