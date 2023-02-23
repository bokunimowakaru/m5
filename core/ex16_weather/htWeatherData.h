// HTTP GETデータ格納用

#ifndef HT_WETHER_DATA
typedef struct {
    byte day;           // 天気予報日
    byte hour;          // 天気予報時刻
    char text[17];      // 予報テキスト
    char disp[9];       // 天気予報・表示用(Fine/Cloudy/Rainy/Snowy)
    signed char code;   // 天気コード(4:Snowy;3:Fine;2:Cloudy;1:Rainy)⇒(4:Snowy;3:Rainy;2:Cloudy;1:Fine)
    int tempH;          // 天気予想温度（最高気温）
    int tempL;          // 天気予想温度（最低気温）
    int codes[3][2];    // 時刻;天気code {{int hour, int code},...}
    byte pops[6][2];    // 時刻;降水確率pop  {{byte hour, byte pop},...}
    int temps[4][2];    // 時刻;気温temps  {{int (hour+24*day), int temp},...}
    int week[7][3];     // 1週間分の 日、天気、降水確率  {{int day, int code, int pop},...}
} HtWetherData;

const char wtrFiles[5][13]={
    "wtr_uknw_jpg", 
    "wtr_fine_jpg", 
    "wtr_clud_jpg", 
    "wtr_rain_jpg", 
    "wtr_snow_jpg"
};

#define HT_WETHER_DATA
#endif // HT_WETHER_DATA

/*
https://www.jma.go.jp/bosai/forecast/data/forecast/130000.json

*/