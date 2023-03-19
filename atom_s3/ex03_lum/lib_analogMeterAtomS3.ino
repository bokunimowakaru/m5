/***********************************************************************
analogMeterInit
analogMeterNeedle

本ソースコードは下記からダウンロードし、国野亘がロガー用に改変しました。
https://github.com/m5stack/M5Stack/blob/master/examples/Advanced/Display/TFT_Meter_linear/TFT_Meter_linear.ino

・M5StickC （128 x 128 サイズ
・M5StickC Plus LCD 240 x 135 (40 x 17) フォント8x6ピクセル

改変部の著作権は国野亘にあります。
配布は自由ですが権利情報の変更は不可。補償や責任を負いません。

Copyright (c) 2019-2023 Wataru KUNINO

元のライセンスについては下記を参照してください。
https://github.com/m5stack/M5Stack/blob/master/LICENSE

m5stack/M5Stack is licensed under the MIT License

2019/12/27

以下、TFT_Meter_linear
*/
/*
 An example analogue meter using a ILI9341 TFT LCD screen

 Needs Font 2 (also Font 4 if using large scale label)

 Make sure all the display driver and pin comnenctions are correct by
 editting the User_Setup.h file in the TFT_eSPI library folder.

 #########################################################################
 ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
 #########################################################################
 
Updated by Bodmer for variable meter size
 */

// Define meter size as 1 for M5.Lcd.rotation(0) or 1.3333 for M5.Lcd.rotation(1)
#define M_SIZE 0.5

#define TFT_GREY 0x5AEB

String analogMeterUnit1="Celsius";
String analogMeterName1="Temp.";
int analogMeterMinVal1 = 0;
int analogMeterMaxVal1 = 40;

float ltx1 = 0;    // Saved x coord of bottom of needle
uint16_t osx1 = M_SIZE*120, osy1 = M_SIZE*120; // Saved x & y coords
int old_analog1 =  -1; // Value last displayed
uint32_t updateTime = 0;       // time for next update
/*
int value[6] = {0, 0, 0, 0, 0, 0};
int old_value[6] = { -1, -1, -1, -1, -1, -1};
int d = 0;

void setup(void) {
  M5.begin();
  // M5.Lcd.setRotation(1);
  // Serial.begin(57600); // For debug
  M5.Lcd.fillScreen(TFT_BLACK);

  analogMeter(); // Draw analogue meter

  updateTime = millis(); // Next update time
}


void loop() {
  if (updateTime <= millis()) {
    updateTime = millis() + 35; // Update emter every 35 milliseconds
 
    // Create a Sine wave for testing
    d += 4; if (d >= 360) d = 0;
    value[0] = 50 + 50 * sin((d + 0) * 0.0174532925);
 
    plotNeedle(value[0], 0); // It takes between 2 and 12ms to replot the needle with zero delay
  }
}
*/

// #########################################################################
//  Draw the analogue meter on the screen
// #########################################################################
void analogMeterInit()
{
  M5.Lcd.setRotation(1);
  M5.Lcd.setTextSize(1);

  // Meter outline (0,0) to (319,83) 320x80
  // M5.Lcd.fillRect(0, 0, 160, 80, TFT_WHITE);
  // M5.Lcd.fillRect(0, 0, 240, 135, TFT_WHITE);
  M5.Lcd.fillRect(0, 0, 128, 68, WHITE);

  M5.Lcd.setTextColor(BLACK);  // Text colour

  // Draw ticks every 5 degrees from -50 to +50 degrees (100 deg. FSD swing)
  for (int i = -50; i < 51; i += 5) {
    // Long scale tick length
    int tl = 15;

    // Coodinates of tick to draw
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (M_SIZE*100 + tl) + M_SIZE*120;
    uint16_t y0 = sy * (M_SIZE*100 + tl) + M_SIZE*155;
    uint16_t x1 = sx * M_SIZE*100 + M_SIZE*120;
    uint16_t y1 = sy * M_SIZE*100 + M_SIZE*155;

    // Coordinates of next tick for zone fill
    float sx2 = cos((i + 5 - 90) * 0.0174532925);
    float sy2 = sin((i + 5 - 90) * 0.0174532925);
    int x2 = sx2 * (M_SIZE*100 + tl) + M_SIZE*120;
    int y2 = sy2 * (M_SIZE*100 + tl) + M_SIZE*155;
    int x3 = sx2 * M_SIZE*100 + M_SIZE*120;
    int y3 = sy2 * M_SIZE*100 + M_SIZE*155;

    // Yellow zone limits
    //if (i >= -50 && i < 0) {
    //  M5.Lcd.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_YELLOW);
    //  M5.Lcd.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_YELLOW);
    //}

    // Green zone limits
    if (i >= 0 && i < 25) {
      M5.Lcd.fillTriangle(x0, y0, x1, y1, x2, y2, GREEN);
      M5.Lcd.fillTriangle(x1, y1, x2, y2, x3, y3, GREEN);
    }

    // Orange zone limits
    if (i >= 25 && i < 50) {
      M5.Lcd.fillTriangle(x0, y0, x1, y1, x2, y2, ORANGE);
      M5.Lcd.fillTriangle(x1, y1, x2, y2, x3, y3, ORANGE);
    }

    // Short scale tick length
    if (i % 25 != 0) tl = 8;

    // Recalculate coords incase tick lenght changed
    x0 = sx * (M_SIZE*100 + tl) + M_SIZE*120;
    y0 = sy * (M_SIZE*100 + tl) + M_SIZE*155;
    x1 = sx * M_SIZE*100 + M_SIZE*120;
    y1 = sy * M_SIZE*100 + M_SIZE*155;

    // Draw tick
    M5.Lcd.drawLine(x0, y0, x1, y1, BLACK);

    // Check if labels should be drawn, with position tweaks
    if (i % 25 == 0) {
      // Calculate label positions
      x0 = sx * (M_SIZE*100 + tl + 10) + M_SIZE*120;
      y0 = sy * (M_SIZE*100 + tl + 10) + M_SIZE*150;
      switch (i / 25) {
        case -2:
        	M5.Lcd.drawCentreString(String(analogMeterMinVal1), x0+8, y0, 1);
        	break;
        case -1:
        	M5.Lcd.drawCentreString(String((analogMeterMinVal1*3+analogMeterMaxVal1)/4), x0, y0 + 2, 1);
        	break;
        case 0:
        	M5.Lcd.drawCentreString(String((analogMeterMinVal1+analogMeterMaxVal1)/2), x0, y0 +2 , 1);
        	break;
        case 1:
        	M5.Lcd.drawCentreString(String((analogMeterMinVal1+analogMeterMaxVal1*3)/4), x0, y0 + 2, 1);
        	break;
        case 2:
        	M5.Lcd.drawCentreString(String(analogMeterMaxVal1), x0, y0, 1);
        	break;
      }
    }

    // Now draw the arc of the scale
    sx = cos((i + 5 - 90) * 0.0174532925);
    sy = sin((i + 5 - 90) * 0.0174532925);
    x0 = sx * M_SIZE*100 + M_SIZE*120;
    y0 = sy * M_SIZE*100 + M_SIZE*155;
    // Draw scale arc, don't draw the last part
    if (i < 50){
		M5.Lcd.drawLine(x0, y0, x1, y1, BLACK);
	}
  }


  M5.Lcd.drawRightString(analogMeterUnit1, M_SIZE*236, M_SIZE*116, 1); // Units at bottom right
  M5.Lcd.drawCentreString(analogMeterName1, M_SIZE*120, M_SIZE*70, 2); // Comment out to avoid font 4

  analogMeterNeedle(0.0,1); // Put meter needle at 0
  M5.Lcd.setCursor(0,0);
}

void analogMeterInit(String unit_S1, String unit_S2, int min_val1, int max_val1){
  analogMeterUnit1 = unit_S1;
  analogMeterName1 = unit_S2;
  analogMeterMinVal1 = min_val1;
  analogMeterMaxVal1 = max_val1;
  analogMeterInit();
}
void analogMeterInit(String unit_S1, int min_val1, int max_val1){
  analogMeterUnit1 = unit_S1;
  analogMeterName1 = unit_S1;
  analogMeterMinVal1 = min_val1;
  analogMeterMaxVal1 = max_val1;
  analogMeterInit();
}

void analogMeterInit(String unit_S1){
  analogMeterUnit1 = unit_S1;
  analogMeterInit();
}

void analogMeterSetName(String unit_S){
		if(unit_S.length()>7) analogMeterName1 = unit_S.substring(0, 7);
		else analogMeterName1 = unit_S;
}


// #########################################################################
// Update needle position
// This function is blocking while needle moves, time depends on ms_delay
// 10ms minimises needle flicker if text is drawn within needle sweep area
// Smaller values OK if text not in sweep area, zero for instant movement but
// does not look realistic... (note: 100 increments for full scale deflection)
// #########################################################################
void analogMeterNeedle(float value_fn, byte ms_delay)
{
  int ch=0;
  M5.Lcd.setTextColor(BLACK, WHITE);
  char buf[8];
  dtostrf(value_fn, 5, 1, buf);
  M5.Lcd.fillRect(3+ch*160, M_SIZE * 116, M_SIZE * 64, 8, WHITE);
  M5.Lcd.drawString(buf, M_SIZE*10+ch*160, M_SIZE*(116), 1);

  float value_f=0., delta=0., min=0., ltx=0.;
  uint16_t osx = M_SIZE*120, osy = M_SIZE*120;
  String analogMeterName;
  int old_analog;

	value_f = value_fn;
	delta = (float)(analogMeterMaxVal1 - analogMeterMinVal1);
	min = analogMeterMinVal1;
	ltx = ltx1;
	osx = osx1;
	osy = osy1;
	old_analog = old_analog1;
	analogMeterName = analogMeterName1;
  
  if( delta != 0.){
	value_f = ( value_f - min ) * 100 / delta;
  }
  int value;
  if(value_f > 0. ) value = (int)(value_f + 0.5);
  else              value = (int)(value_f - 0.5);

  if (value < -7) value = -7; // Limit value to emulate needle end stops
  if (value > 107) value = 107;

  // Move the needle until new value reached
  while (!(value == old_analog)) {
    if (old_analog < value) old_analog++;
    else old_analog--;

    if (ms_delay <= 0) old_analog = value; // Update immediately if delay is 0

    float sdeg = map(old_analog, -10, 110, -150, -30); // Map value to angle
    // Calcualte tip of needle coords
    float sx = cos(sdeg * 0.0174532925);
    float sy = sin(sdeg * 0.0174532925);

    // Calculate x delta of needle start (does not start at pivot point)
    float tx = tan((sdeg + 90) * 0.0174532925);

    // Erase old needle image
    M5.Lcd.drawLine(M_SIZE*(120 + 20 * ltx - 1) + ch * 160, M_SIZE*(155 - 20), osx - 1 + ch * 160, osy+1, WHITE);
    M5.Lcd.drawLine(M_SIZE*(120 + 20 * ltx) + ch * 160, M_SIZE*(155 - 20), osx + ch * 160, osy, WHITE);
    M5.Lcd.drawLine(M_SIZE*(120 + 20 * ltx + 1) + ch * 160, M_SIZE*(155 - 20), osx + 1 + ch * 160, osy+1, WHITE);

    // Re-plot text under needle
    M5.Lcd.setTextColor(BLACK);
    M5.Lcd.fillRect(M_SIZE*70 + ch * 160, M_SIZE*70, M_SIZE*100, 16,WHITE);
    M5.Lcd.drawCentreString(analogMeterName, M_SIZE*120 + ch * 160, M_SIZE*70, 2); // // Comment out to avoid font 4

    // Store new needle end coords for next erase
    ltx = tx;
    osx = M_SIZE*(sx * 98 + 120);
    osy = M_SIZE*(sy * 98 + 155);

    // Draw the needle in the new postion, magenta makes needle a bit bolder
    // draws 3 lines to thicken needle
    M5.Lcd.drawLine(M_SIZE*(120 + 20 * ltx - 1) + ch * 160, M_SIZE*(155 - 20), osx - 1 + ch * 160, osy+1, RED);
    M5.Lcd.drawLine(M_SIZE*(120 + 20 * ltx) + ch * 160, M_SIZE*(155 - 20), osx + ch * 160, osy, MAGENTA);
    M5.Lcd.drawLine(M_SIZE*(120 + 20 * ltx + 1) + ch * 160, M_SIZE*(155 - 20), osx + 1 + ch * 160, osy+1, RED);

    // Slow needle down slightly as it approaches new postion
    if (abs(old_analog - value) < 10) ms_delay += ms_delay / 5;

    // Wait before next update
    delay(ms_delay);
  }

	ltx1 = ltx;
	osx1 = osx;
	osy1 = osy;
	old_analog1 = old_analog;

  M5.Lcd.setTextColor(WHITE);
}

void analogMeterNeedle(float value_fn){
	analogMeterNeedle(value_fn, 35);
}
