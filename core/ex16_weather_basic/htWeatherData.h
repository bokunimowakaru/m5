// HTTP GETデータ格納用

#ifndef HT_WETHER_DATA
typedef struct {
    byte day;           // 天気予報日
    byte hour;          // 天気予報時刻
    char text[17];      // 予報テキスト
    char disp[9];       // 天気予報・表示用(Fine/Cloudy/Rainy/Snowy)
    char code;          // 天気コード(4:Snowy;3:Fine;2:Cloudy;1:Rainy)⇒(4:Snowy;3:Rainy;2:Cloudy;1:Fine)
    int tempH;          // 天気予想温度（最高気温）
    int tempL;          // 天気予想温度（最低気温）
    int codes[3][2];    // 時刻;天気code
    byte pops[6][2];    // 時刻;降水確率pops
    char temps[4][2];   // 時刻;気温temps
    int week[7][3];     // 1週間分の 日、天気、降水確率
} HtWetherData;
#define HT_WETHER_DATA
#endif // HT_WETHER_DATA
