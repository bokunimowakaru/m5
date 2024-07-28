/*******************************************************************************
Graph表示モジュール for M5Stack
********************************************************************************

・液晶ディスプレイにグラフを表示します。
・320 x 216 -> 320 x 110 (実サイズは108)
                                          Copyright (c) 2019-2020 Wataru KUNINO
*******************************************************************************/

#include <M5Stack.h>
#define TFT_GREY 0x5AEB
#define LgaY 84	// Y軸、位置シフト加算用
#define LggY 2	// Y軸、縮小率
#define GVals 2 // 表示データ数

int lineGraphMinVal = 0;
int lineGraphMaxVal = 100;
byte lineGraphVal[GVals][280];
int lineGraphVal_n=0;
uint16_t colors[10] = {TFT_BLACK,TFT_RED,TFT_MAROON,TFT_NAVY,TFT_DARKGREY,TFT_BLUE,TFT_GREEN,TFT_PURPLE,TFT_OLIVE,TFT_DARKCYAN};

void lineGraphCls(){
	M5.Lcd.setTextColor(TFT_BLACK);
	M5.Lcd.fillRect(2, 4/LggY + LgaY, 316, 208/LggY, TFT_WHITE);
	for (int i = 0; i <= 10; i++) {
		int y = 8 + i * 20;
		M5.Lcd.drawLine(30, y/LggY + LgaY, 310, y/LggY + LgaY, TFT_LIGHTGREY);
		y = 200 - i * 20 + 4;
		if(i == 0) y-= 4;
		if(i == 10) y++;
		M5.Lcd.drawRightString(String(map(i,0,10,lineGraphMinVal,lineGraphMaxVal)), 28, y/LggY + LgaY, 1);
	}
	for (int i = 0; i <= 14; i++) {
		int x = 30 + i * 20;
		int y = 200;
		M5.Lcd.drawLine(x, 8/LggY + LgaY, x, 208/LggY + LgaY, TFT_LIGHTGREY);
		if(i % 3 == 1){
			M5.Lcd.drawCentreString(String(240 - (60*(i/3)) ), x, y/LggY + LgaY, 1);
		}
	}
}

void lineGraphInit(){
	M5.Lcd.setTextSize(1);
	M5.Lcd.fillRect(0, LgaY, 320, 216/LggY, TFT_GREY);
	lineGraphCls();
	M5.Lcd.setCursor(0, 110 + 84);
	M5.Lcd.setTextColor(TFT_WHITE);
}

void lineGraphInit(int min_val, int max_val){
	lineGraphMinVal = min_val;
	lineGraphMaxVal = max_val;
	lineGraphInit();
}

// d=GVals-1のときだけ時間を進めて描画。それ以外はデータ代入のみ
void lineGraphPlot(float value_f, int d){
	float delta = (float)(lineGraphMaxVal - lineGraphMinVal);
	if( delta != 0.){
		value_f = ( value_f - (float)lineGraphMinVal ) * 200. / delta;
	}
	int value;
	if(value_f > 0. ) value = (int)(value_f + 0.5);
	else              value = (int)(value_f - 0.5);

	if(value < -4) value = -4;
	if(value > 204) value = 204;
	lineGraphVal[d][lineGraphVal_n] = (byte)(208 - value);
	if(d != GVals - 1) return;
	
	if(lineGraphVal_n > 0){
		if(lineGraphVal_n >= 279){
			lineGraphCls();
			for(int dd=0 ; dd < GVals; dd++)for(int i=0;i<260;i++) lineGraphVal[dd][i] = lineGraphVal[dd][i+20];
			for(int i=1;i<259;i++){
				int x = 30 + i;
				for(int dd=0 ; dd < GVals; dd++){
					int y = (int)lineGraphVal[dd][i];
					M5.Lcd.drawLine(x, y/LggY + LgaY, x-1, (int)lineGraphVal[dd][i-1]/LggY + LgaY, (int)colors[dd]);
				}
			}
			lineGraphVal_n = 259;
		}
		int i = lineGraphVal_n;
		int x = 30 + i;
		for(int dd=0 ; dd < GVals; dd++){
			int y = (int)lineGraphVal[dd][i];
			M5.Lcd.drawLine(x, y/LggY + LgaY, x-1, (int)lineGraphVal[dd][i-1]/LggY + LgaY, (int)colors[dd]);
		}
	}
	lineGraphVal_n++;
	M5.Lcd.setTextColor(TFT_WHITE);
}
