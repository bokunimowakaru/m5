/*******************************************************************************
Graph表示モジュール for M5Stack
********************************************************************************

・e-paper ディスプレイにグラフを表示します。
・320 x 110 (実サイズは108) ->  200 x 110　(0,16,200,112,0)

                                          Copyright (c) 2019-2023 Wataru KUNINO
********************************************************************************
本ソースコードの作成にあたり、下記の文献を参考にしました。

【参考文献】
M5Stack Arduino Library API 情報(eink)：
https://docs.m5stack.com/en/api/coreink/eink_api
********************************************************************************
引用コード：
https://github.com/bokunimowakaru/m5/blob/master/core/ex51_wgbt_ir_out/lib_lineGraph4log.ino
*******************************************************************************/

#include <M5CoreInk.h>                          // M5Stack用ライブラリ組み込み
int LgaY=16;    // Y軸、位置シフト加算用
#define LggY 2  // Y軸、縮小率
#define GVals 3 // 表示データ数 最大 10

Ink_Sprite *InkGraphSprite;                     // M5Ink描画用ポインタ
int lineGraphMinVal = 0;
int lineGraphMaxVal = 100;
RTC_DATA_ATTR byte lineGraphVal[GVals][160];    // RTC MEM 8KBまで, 320-40=280 -> 200-40=160
RTC_DATA_ATTR int lineGraphVal_n;
uint16_t line_w[10] = {100,100,100,100,100,100,100,100,100,100};

void drawLine(int x1, int y1, int x2, int y2, int width=100);
void drawLine(int x1, int y1, int x2, int y2, int width){
    int len = sqrt(pow((double)(x2-x1),2)+pow((double)(y2-y1),2)) + 0.5;
    int step = 100 / width;
    if(x1>x2){
        int x = x1; x1 = x2; x2 =x;
        int y = y1; y1 = y2; y2 =y;
    }
    double rad = atan((double)(y2-y1)/(double)(x2-x1));
    if(step <= 1){
        step = 1;
    }
    if(width < 100) width = 100;
    for(int i=0; i<len; i+=step){
        for(int w = 1-(width/100); w < width/100; w++){
            int dx = (int)((double)i * cos(rad) - (double)w * sin(rad) + 0.5);
            int dy = (int)((double)i * sin(rad) + (double)w * cos(rad) + 0.5);
            InkGraphSprite->drawPix(x1+dx,y1+dy,0);
        }
    }
}

void lineGraphCls(){
    InkGraphSprite->FillRect(0,LgaY,200,112,1);;
    for (int i = 0; i <= 10; i++) {
        int y = 8 + i * 20;     // y:4～104 ＋16 20～120
        drawLine(30, y/LggY+LgaY, 190, y/LggY+LgaY, 50);
        y = 200 - i * 20 - 4;
        if(i == 0) y -= 4;
        if(i == 10) y += 6;
        if(i % 2 == 0){
            // String S = String(map(i,0,10,lineGraphMinVal,lineGraphMaxVal));
            // InkGraphSprite->drawString(28 - 8 * S.length(), y/LggY+LgaY, S);
            char s[9];
            snprintf(s,9,"%d",map(i,0,10,lineGraphMinVal,lineGraphMaxVal));
            InkGraphSprite->drawString(28 - 8 * strlen(s), y/LggY+LgaY, s);
        }
    }
    for (int i = 0; i <= 8; i++) {
        int x = 30 + i * 20;
        int y = 200;
        drawLine(x, 8/LggY+LgaY, x, 208/LggY+LgaY, 50);
        if(i % 3 == 1){
            // String S = String(120 - (60*(i/3)));
            // InkGraphSprite->drawString(x - 4 * S.length(), y/LggY+LgaY, S);
            char s[9];
            snprintf(s,9,"%d",120 - (60*(i/3)));
            InkGraphSprite->drawString(x - 4 * strlen(s), y/LggY - 2 + LgaY, s);
        }
    }
    drawLine(0, LgaY, 199, LgaY, 50);
    drawLine(0, 111+LgaY, 199, 111+LgaY, 50);
    drawLine(0, LgaY, 0, 111+LgaY, 50);
    drawLine(199, 0+LgaY, 199, 111+LgaY, 50);
//  delay(240);
//  InkGraphSprite->pushSprite();
}

void lineGraphRedraw(){
    if(lineGraphVal_n > 0){
        for(int i=1;i<lineGraphVal_n;i++){
            int x = 30 + i;
            for(int dd=0 ; dd < GVals; dd++){
                int y = (int)lineGraphVal[dd][i];
                drawLine(x, y/LggY+LgaY, x-1, (int)lineGraphVal[dd][i-1]/LggY+LgaY, (int)line_w[dd]);
            }
        }
    }
}

void lineGraphCls(Ink_Sprite *sprite, int y){
    InkGraphSprite = sprite;
    LgaY = y;
    lineGraphCls();
}

void lineGraphCls(Ink_Sprite *sprite, int y, int min_val, int max_val){
    InkGraphSprite = sprite;
    LgaY = y;
    lineGraphMinVal = min_val;
    lineGraphMaxVal = max_val;
    lineGraphCls();
}

void lineGraphInit(Ink_Sprite *sprite, int y){
    InkGraphSprite = sprite;
    LgaY = y;
    lineGraphCls();
    lineGraphVal_n=0;
}

void lineGraphInit(Ink_Sprite *sprite, int y, int min_val, int max_val){
    lineGraphMinVal = min_val;
    lineGraphMaxVal = max_val;
    lineGraphInit(sprite, y);
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
        if(lineGraphVal_n >= 159){  // 280 -> 160
            lineGraphCls();
            for(int dd=0 ; dd < GVals; dd++)for(int i=0;i<140;i++) lineGraphVal[dd][i] = lineGraphVal[dd][i+20];
            for(int i=1;i<139;i++){ // 280 -> 160
                int x = 30 + i;
                for(int dd=0 ; dd < GVals; dd++){
                    int y = (int)lineGraphVal[dd][i];
                    drawLine(x, y/LggY+LgaY, x-1, (int)lineGraphVal[dd][i-1]/LggY+LgaY, (int)line_w[dd]);
                }
            }
            lineGraphVal_n = 139;   // 280 -> 160
        }
        if(lineGraphVal_n > 0){
            int i = lineGraphVal_n;
            int x = 30 + i;
            for(int dd=0 ; dd < GVals; dd++){
                int y = (int)lineGraphVal[dd][i];
                drawLine(x, y/LggY+LgaY, x-1, (int)lineGraphVal[dd][i-1]/LggY+LgaY, (int)line_w[dd]);
            }
        }
    }
    lineGraphVal_n++;
    Serial.printf("lineGraphVal_n = %d\n",lineGraphVal_n); // debug
}

void lineGraphPush(){
    InkGraphSprite->pushSprite();
}
