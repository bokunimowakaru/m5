/*******************************************************************************
HTMLコンテンツ取得

# 【重要なご注意】
# 本ソフトウェアの利用に関して、筆者(国野 亘)は、責任を負いません。
# 気象業務法や、下記の予報業務許可に関する情報、上記参考文献の注意事項を
# 良く読んでから利用ください。
# 気象業務法   https://www.jma.go.jp/jma/kishou/info/ml-17.html
# 予報業務許可 https://www.jma.go.jp/jma/kishou/minkan/q_a_m.html

地域設定：
    ・#define CITY_ID に地域コードを設定してください。
            # 気象庁=130000(東京地方など)
            # 大阪管区気象台=270000(大阪府など)
            # 京都地方気象台=260000(南部など)
            # 横浜地方気象台=140000(東部など)
            # 銚子地方気象台=120000(北西部など)
            # 名古屋地方気象台=230000(西部など)
            # 福岡管区気象台=400000(福岡地方など)

◆天気コード変更版◆

                                          Copyright (c) 2016-2023 Wataru KUNINO
********************************************************************************
参考文献
天気予報 API（livedoor 天気互換サービス）, https://weather.tsukumijima.net/
天気予報 API サービスのソースコード, https://github.com/tsukumijima/weather-api
同上, https://github.com/tsukumijima/weather-api/blob/master/app/Models/Weather.php
Yahoo!天気・災害, https://weather.yahoo.co.jp/weather/rss/
Yahoo!サービスの利用規約, https://about.yahoo.co.jp/docs/info/terms/
HTTPコンテンツの連続受信方法, https://github.com/espressif/arduino-esp32/blob\
/master/libraries/HTTPClient/examples/StreamHttpClient/StreamHttpClient.ino
*******************************************************************************/

#include "htWeatherData.h"
#include <WiFi.h>                           // ESP32用WiFiライブラリ
#include <WiFiClientSecure.h>               // TLS(SSL)通信用ライブラリ
#include <HTTPClient.h>                     // HTTP通信用ライブラリ

#define TIMEOUT 6000                        // タイムアウト 6秒
#define BUF_N 271 * 2                       // バッファサイズ 2の倍数 128以上

// 関数宣言 struct利用関数を宣言しておく(Arduinoの自動関数宣言回避用)
HtWetherData *getWeather(int city);         // 天気情報取得

HtWetherData _ht_data;

int httpGetBufferedWeather(char *out, int out_len, int data_number){
// 注意 out_lenはバッファサイズではなく、応答最大文字サイズ
    if(_ht_data.day == 0) return 0;
    memset(out,0,out_len+1);
    switch(data_number){
        case 0:
            snprintf(out,out_len+1,"%d,%d,%d,%s,%s",_ht_data.day,_ht_data.tempH,_ht_data.tempL,_ht_data.disp,_ht_data.text);
            break;
        case 1:
            snprintf(out,out_len+1,"%d",_ht_data.day);
            break;
        case 2:
            snprintf(out,out_len+1,"%d",_ht_data.tempH);
            break;
        case 3:
            snprintf(out,out_len+1,"%d",_ht_data.tempL);
            break;
        case 4:
            snprintf(out,out_len+1,"%s",_ht_data.disp);
            break;
        case 5:
            snprintf(out,out_len+1,"%s",_ht_data.text);
            break;
        default:
            snprintf(out,out_len+1,"%s %d/%d C",_ht_data.disp,_ht_data.temps[0][1],_ht_data.temps[1][1]);
            break;
    }
    #ifdef DEBUG_HTTPG
        Serial.print(strlen(out));
        Serial.print(" Bytes \"");
        Serial.print(out);
        Serial.println("\"");
    #endif
    return strlen(out);
}

int httpGetBufferedWeather(char *out, int out_len){
    return httpGetBufferedWeather(out, out_len, -1);
}

int httpGetBufferedWeatherCode(){
    return _ht_data.code;
}

int httpGetBufferedTemp(){
    return (_ht_data.tempL + _ht_data.tempH)/2;
}

int httpGetBufferedTempH(){
    return _ht_data.tempH;
}

int httpGetBufferedTempL(){
    return _ht_data.tempL;
}

char* _search(char *cp, const char *key, boolean check){
    delay(10);
    char *cp2 = strstr(cp, key);
    if(check && ((cp2 - cp) >= BUF_N/2)) cp2=0;
    if(cp2) cp2 += strlen(key);
    return cp2;
}

char* _search(char *cp, const char *key){
    return _search(cp, key, 0);
}


void http_debug_step(int step, unsigned long time){
    Serial.printf("[%04d]DEBUG step %02d -------------------\n", (millis()-time)%10000, step);
}

HtWetherData *getWeather(int city){
	char s[17];
	httpGetWeather(city, _ht_data.text, 16, -1);
	return &_ht_data;
}


int httpGetWeather(int city, char *out, int out_len, int data_number){
// malloc使用 【解放忘れに注意】/////////////////////////////////////////
    int i,dist,len,ret=0,size,done;
    char c;                        // 文字変数、s=汎用
    char *cp,*cp2;
    unsigned long time=millis();            // 時間測定用
    
    if(city<0 || city>=1000000) return 0;
    
    char *s = (char *) malloc(BUF_N + 1);
    if(!s) return 0;
    s[BUF_N]=0;

    memset(out,0,out_len);
    _ht_data.day=0;
    _ht_data.code=0;
    _ht_data.tempH=0;
    _ht_data.tempL=0;
    memset(_ht_data.text,0,17);
    memset(_ht_data.disp,0,9);
    snprintf(s,BUF_N,"https://www.jma.go.jp/bosai/forecast/data/forecast/%d.json",city);
    
    WiFiClientSecure client;                // TLS/TCP/IP接続部の実体を生成
    // client.setCACert(rootCACertificate); // ルートCA証明書を設定
    client.setInsecure();                   // サーバ証明書を確認しない
    HTTPClient https;                       // HTTP接続部の実体を生成
    https.begin(client, String(s));         // 初期化と接続情報の設定
    https.setTimeout(TIMEOUT);

    Serial.print("URL       : ");
    Serial.println(s);
    Serial.println("Recieving contents...");
    delay(1);
    
    int httpCode = https.GET();                 // HTTP接続の開始
    Serial.print("httpCode  : ");
    Serial.println(httpCode);                   // httpCodeをシリアル出力
    // Serial.println(s);                       // 受信結果をシリアル出力
    
    len = https.getSize();
    Serial.print("Length    : ");
    Serial.println(len);
    delay(1);

    if(httpCode != 200 || len < 128){           // HTTP接続に失敗したとき
        https.end();                            // HTTPクライアントの処理を終了
        client.stop();                          // TLS(SSL)通信の停止
        Serial.println("ERROR: Failed to connect or length < 128");
        free(s);
        return 0;
    }


//  https.getString().toCharArray(s, 2048);     // 受信結果を変数sへ代入
//  size=strlen(s);
    WiFiClient * stream = https.getStreamPtr(); // get tcp stream
    delay(10);
    size = stream->available();
    len -= stream->readBytes(s+BUF_N/2, ((size > BUF_N/2) ? BUF_N/2 : size));
    s[BUF_N]='\0';
    done = 0;
    int done_prev=-1;

    // 解析処理 weathers
    while(https.connected() && done_prev != done){
        if(size > 0){
            memcpy(s,s+BUF_N/2,BUF_N/2);
            len -= stream->readBytes(s+BUF_N/2, ((size > BUF_N/2) ? BUF_N/2 : size));
            s[BUF_N]='\0';
        }else done_prev = done;
        cp = s;
        int target = 0;
        while(cp - s < BUF_N/2){
            
            // 0 親レポート／報告時刻
            if(done == target){
                http_debug_step(done,time);
                cp = _search(cp, "\"reportDatetime\"",1);
                if(!cp) break;
                cp = _search(cp, "-");
                if(cp) cp = _search(cp, "-");
                if(cp){
                    _ht_data.day = (byte)atoi(cp);
                    Serial.print("  wthDay  : ");
                    Serial.println(_ht_data.day);
                    cp = _search(cp, "T");
                    if(cp){
                        _ht_data.hour = (byte)atoi(cp);
                        Serial.print("  wthHour : ");
                        Serial.println(_ht_data.hour);
                        done++;
                    }
                }
            }
            if(done_prev < 0 && (!cp || cp - s >= BUF_N/2)) break;
            
            target++;   // 1 親レポート／データ時刻
            if(done == target){
                http_debug_step(done,time);
                for(i=0;i<3;i++) _ht_data.codes[i][0] = -1;
                cp = _search(cp, "\"timeDefines\"",1);
                if(!cp) break;
                cp2 =_search(cp, "]"); //128バイト以内なので見つかるはず
                if(!cp2) cp2 = s + BUF_N;
                //Serial.printf("DEBUG len timeDefines to ] = %d (cp2 = %d)\n",cp2-cp,cp2);
                
                Serial.print("  wthTime : ");
                for(i=0;i<3;i++){
                    cp = _search(cp, "T");
                    if(!cp || cp >= cp2 ){
                        cp = cp2;
                        break; // for i
                    }
                    _ht_data.codes[i][0] = atoi(cp);
                    Serial.print(_ht_data.codes[i][0]);
                    if(i<2)Serial.print(", ");
                }
                Serial.println();
                done++;
            }
            if(done_prev < 0 && (!cp || cp - s >= BUF_N/2)) break;
            
            target++;   // 2 親レポート／天気コード
            if(done == target){
                http_debug_step(done,time);
                for(i=0;i<3;i++) _ht_data.codes[i][1] = 0;
                cp = _search(cp, "\"weatherCodes\"",1);
                if(!cp) break;
                cp2 =_search(cp, "]");
                if(!cp2) cp2 = s + BUF_N;
                //Serial.printf("DEBUG len weatherCodes to ] = %d (cp2 = %d)\n",cp2-cp,cp2);
                Serial.print("  wthCode : ");
                for(i=0;i<3;i++){
                    cp = _search(cp, "\"");
                    if(!cp || cp >= cp2 ){
                        cp = cp2;
                        break; // for i
                    }
                    _ht_data.codes[i][1] = atoi(cp);
                    Serial.print(_ht_data.codes[i][1]);
                    if(i<2)Serial.print(", ");
                    cp = _search(cp, "\"");
                    if(!cp){
                        cp = cp2;
                        break; // for i
                    }
                }
                Serial.println();
                done++;
            }
            if(done_prev < 0 && (!cp || cp - s >= BUF_N/2)) break;
            
            target++;   // 3 親レポート／天気・日本語
            if(done == target){
                http_debug_step(done,time);
                /*
                cp = strstr(cp,"\"weathers\"");
                if(!cp || cp - s >= BUF_N/2) break;
                cp += 13;
                dist=0;
                _ht_data.code=0;
                cp2 = strstr(cp, "晴れ");
                if(cp2){
                    dist = (int)(cp2 - cp);
                    strcpy(_ht_data.text, "晴");
                    strcpy(_ht_data.disp,"Fine");
                    _ht_data.code=1;
                }
                cp2 = strstr(cp, "くもり");
                if(cp2 && (int)(cp2 - cp) < dist){
                    dist = (int)(cp2 - cp);
                    strcpy(_ht_data.text, "曇");
                    strcpy(_ht_data.disp,"Cloudy");
                    _ht_data.code=2;
                }
                cp2 = strstr(cp, "雨");
                if(cp2 && (int)(cp2 - cp) < dist){
                    dist = (int)(cp2 - cp);
                    strcpy(_ht_data.text, "雨");
                    strcpy(_ht_data.disp,"Rainy");
                    _ht_data.code=3;
                }
                cp2 = strstr(cp, "雪");
                if(cp2 && (int)(cp2 - cp) < dist){
                    dist = (int)(cp2 - cp);
                    strcpy(_ht_data.text, "雪");
                    strcpy(_ht_data.disp,"Snowy");
                    _ht_data.code=4;
                }
                if(_ht_data.code==0){
                    strcpy(_ht_data.text, "－");
                    strcpy(_ht_data.disp,"Unknown");
                }
                */
                i = _ht_data.codes[0][1] / 100;
                if(i==1){
                    strcpy(_ht_data.text, "晴");
                    strcpy(_ht_data.disp,"Fine");
                    _ht_data.code=1;
                }else if(i==2){
                    strcpy(_ht_data.text, "曇");
                    strcpy(_ht_data.disp,"Cloudy");
                    _ht_data.code=2;
                }else if(i==3){
                    dist = (int)(cp2 - cp);
                    strcpy(_ht_data.text, "雨");
                    strcpy(_ht_data.disp,"Rainy");
                    _ht_data.code=3;
                }else if(i==4){
                    strcpy(_ht_data.text, "雪");
                    strcpy(_ht_data.disp,"Snowy");
                    _ht_data.code=4;
                }else{
                    strcpy(_ht_data.text, "－");
                    strcpy(_ht_data.disp,"Unknown");
                    _ht_data.code=0;
                }
                Serial.print("  weather : ");
                Serial.print(_ht_data.disp);
                Serial.print(" / ");
                Serial.println(_ht_data.text);
                done++;
            }
            if(done_prev < 0 && (!cp || cp - s >= BUF_N/2)) break;
            
            target++;   // 4 親レポート／降水確率用の時刻
            if(done == target){
                http_debug_step(done,time);
                for(i=0;i<6;i++) _ht_data.pops[i][0] = 255;
                cp = _search(cp, "\"timeDefines\"",1);
                if(!cp) break;
                cp2 =_search(cp, "]");
                if(!cp2) cp2 = s + BUF_N;
                //Serial.printf("DEBUG len timeDefines to ] = %d (cp2 = %d)\n",cp2-cp,cp2);
                Serial.print("  wthTime : ");
                i = 0;
                for(;i<6;i++){
                    //Serial.printf("DEBUG len s=%d, len cp=%d\n",strlen(s),strlen(cp));
                    cp = strchr(cp, 'T');
                    if(cp) cp++;
                    //Serial.printf("DEBUG len cp=%d (cp = %d)\n",strlen(cp),cp);
                    if(!cp || cp >= cp2 ){
                        cp = cp2;
                        Serial.print("/break/");
                        break; // for i
                    }
                    _ht_data.pops[i][0] = (byte)atoi(cp);
                    Serial.print(_ht_data.pops[i][0]);
                    if(i<5)Serial.print(", ");
                }
                Serial.println();
                done++;
            }
            if(done_prev < 0 && (!cp || cp - s >= BUF_N/2)) break;

            target++;   // 5 親レポート／降水確率
            if(done == target){
                http_debug_step(done,time);
                for(i=0;i<6;i++) _ht_data.pops[i][1] = 255;
                cp = _search(cp, "\"pops\"",1);
                if(!cp) break;
                cp2 =_search(cp, "]");
                if(!cp2) cp2 = s + BUF_N;
                Serial.print("  wthPop  : ");
                for(i=0;i<6;i++){
                    cp = _search(cp, "\"");
                    if(!cp || cp >= cp2 ){
                        Serial.print("/break/");
                        cp = cp2;
                        break; // for i
                    }
                    _ht_data.pops[i][1] = (byte)atoi(cp);
                    Serial.print(_ht_data.pops[i][1]);
                    if(i<5)Serial.print(", ");
                    cp = _search(cp, "\"");
                    if(!cp){
                        cp = cp2;
                        break; // for i
                    }
                }
                Serial.println();
                done++;
            }
            if(done_prev < 0 && (!cp || cp - s >= BUF_N/2)) break;

            target++;   // 6 親レポート／気温用の時刻
            if(done == target){
                http_debug_step(done,time);
                for(i=0;i<4;i++) _ht_data.temps[i][0] = (char)-1;
                cp = _search(cp, "\"timeDefines\"",1);
                if(!cp) break;
                cp2 =_search(cp, "]");
                if(!cp2) cp2 = s + BUF_N;
                Serial.print("  wthTime : ");
                for(i=0;i<4;i++){
                    cp = strchr(cp, 'T');
                    if(cp) cp++;
                    if(!cp || cp >= cp2 ){
                        cp = cp2;
                        Serial.print("/break/");
                        break; // for i
                    }
                    _ht_data.temps[i][0] = (char)atoi(cp);
                    Serial.print((int)_ht_data.temps[i][0]);
                    if(i<3)Serial.print(", ");
                }
                Serial.println();
                done++;
            }
            if(done_prev < 0 && (!cp || cp - s >= BUF_N/2)) break;

            target++;   // 7 親レポート／気温
            if(done == target){
                http_debug_step(done,time);
                for(i=0;i<4;i++) _ht_data.temps[i][1] = (char)-1;
                cp = _search(cp, "\"temps\"",1);
                if(!cp) break;
                cp2 =_search(cp, "]");
                if(!cp2) cp2 = s + BUF_N;
                Serial.print("  wthTemp : ");
                for(i=0;i<4;i++){
                    cp = _search(cp, "\"");
                    if(!cp || cp >= cp2 ){
                        cp = cp2;
                        Serial.print("/break/");
                        break; // for i
                    }
                    _ht_data.temps[i][1] = (char)atoi(cp);
                    Serial.print((int)_ht_data.temps[i][1]);
                    if(i<3)Serial.print(", ");
                    cp = _search(cp, "\"");
                    if(!cp){
                        cp = cp2;
                        break; // for i
                    }
                }
                Serial.println();
                done++;
            }
            if(done_prev < 0 && (!cp || cp - s >= BUF_N/2)) break;

            target++;   // 8 子レポート(先頭)／報告時刻
            if(done == target){
                http_debug_step(done,time);
                cp = _search(cp, "\"reportDatetime\"",1);
                if(!cp) break;
                cp = _search(cp, "-");
                if(cp) cp = _search(cp, "-");
                if(cp){
                    int h=-1;
                    i = atoi(cp);
                    Serial.print("  wthDay  : ");
                    Serial.println(i);
                    cp = _search(cp, "T");
                    if(cp){
                        h = (byte)atoi(cp);
                        Serial.print("  wthHour : ");
                        Serial.println(h);
                    }
                    if(i == _ht_data.day && _ht_data.hour == h){
                        // Serial.println("Passed");
                        done++;
                    }else{
                        Serial.println("FAILED: parent's date not meet child's");
                    }
                }
            }
            if(done_prev < 0 && (!cp || cp - s >= BUF_N/2)) break;

            target++;   // 9 子レポート／データ日
            if(done == target){
                http_debug_step(done,time);
                for(i=0;i<7;i++) _ht_data.week[i][0] = -1;
                cp = _search(cp, "\"timeDefines\"",1);
                if(!cp) break;
                cp2 =_search(cp, "]");
                if(!cp2) cp2 = s + BUF_N;
                Serial.print("  wthDay  : ");
                i = 0;
                for(;i<7;i++){
                    cp = strchr(cp, '-');
                    if(cp){
                        cp++;
                        cp = strchr(cp, '-');
                        if(cp) cp++;
                    }
                    if(!cp || cp >= cp2 ){
                        cp = cp2;
                        Serial.print("/break/");
                        break; // for i
                    }
                    _ht_data.week[i][0] = atoi(cp);
                    Serial.print(_ht_data.week[i][0]);
                    if(i<6)Serial.print(", ");
                }
                Serial.println();
                done++;
            }
            if(done_prev < 0 && (!cp || cp - s >= BUF_N/2)) break;

            target++;   // 10 子レポート／天気コード
            if(done == target){
                http_debug_step(done,time);
                for(i=0;i<7;i++) _ht_data.week[i][1] = 0;
                cp = _search(cp, "\"weatherCodes\"",1);
                if(!cp) break;
                cp2 =_search(cp, "]");
                if(!cp2) cp2 = s + BUF_N;
                Serial.print("  wthCode : ");
                for(i=0;i<7;i++){
                    cp = _search(cp, "\"");
                    if(!cp || cp >= cp2 ){
                        cp = cp2;
                        Serial.print("/break/");
                        break; // for i
                    }
                    _ht_data.week[i][1] = atoi(cp);
                    Serial.print(_ht_data.week[i][1]);
                    if(i<6)Serial.print(", ");
                    cp = _search(cp, "\"");
                    if(!cp){
                        cp = cp2;
                        break; // for i
                    }
                }
                Serial.println();
                done++;
            }
            if(done_prev < 0 && (!cp || cp - s >= BUF_N/2)) break;

            target++;   // 11 子レポート／降水確率
            if(done == target){
                http_debug_step(done,time);
                for(i=0;i<7;i++) _ht_data.week[i][2] = -1;
                cp = _search(cp, "\"pops\"",1);
                if(!cp) break;
                cp2 =_search(cp, "]");
                if(!cp2) cp2 = s + BUF_N;
                Serial.print("  wthPop  : ");
                for(i=0;i<7;i++){
                    cp = _search(cp, "\"");
                    if(!cp || cp >= cp2 ){
                        cp = cp2;
                        Serial.print("/break/");
                        break; // for i
                    }
                    _ht_data.week[i][2] = atoi(cp);
                    Serial.print(_ht_data.week[i][2]);
                    if(i<6)Serial.print(", ");
                    cp = _search(cp, "\"");
                    if(!cp){
                        cp = cp2;
                        break; // for i
                    }
                }
                Serial.println();
                done++;
            }
            if(done_prev < 0 && (!cp || cp - s >= BUF_N/2)) break;

            target++;   // 12
            if(done == target){
                http_debug_step(done,time);
                cp = strstr(cp,"\"tempsMin\"");
                if(done_prev < 0 && (!cp || cp - s >= BUF_N/2)) break;
                cp += 16;
                _ht_data.tempL=atoi(cp);
                Serial.print("   temp_L : ");
                Serial.println(_ht_data.tempL);
                done++;
            }
            if(done_prev < 0 && (!cp || cp - s >= BUF_N/2)) break;

            target++;   // 13
            if(done == target){
                http_debug_step(done,time);
                cp = strstr(s,"\"tempsMax\"");
                if(done_prev < 0 && (!cp || cp - s >= BUF_N/2)) break;
                cp += 16;
                _ht_data.tempH=atoi(cp);
                Serial.print("   temp_H : ");
                Serial.println(_ht_data.tempH);
                done++;
            }
            
            if(done_prev < 0 && (!cp || cp - s >= BUF_N/2)) break;
            target++;   // 14
            if(done == target) break;
        }
        if(done == 14){
            http_debug_step(done,time);
            break;
        }
        delay(10);
        size = stream->available();
    }
    https.end();                                // HTTPクライアントの処理を終了
    client.stop();                              // TLS(SSL)通信の停止
    delay(1);
    
    Serial.print(millis()-time);
    Serial.println("ms, Done");
    
    httpGetBufferedWeather(out, out_len, data_number);
    free(s);
    return httpGetBufferedWeatherCode();
}

/*
    以下はYahoo!天気・災害のRSS配信情報(配信終了)を使用する場合の処理部です。
    すでにサービスが停止しています。
*/

// ご注意：
//    ・Yahoo!天気・災害の情報を商用で利用する場合はYahoo! Japanの承諾が必要です。
//    ・Yahoo!サービスの利用規約にしたがって利用ください。

/*
int httpGetWeather(int city, char *out, int out_len, int data_number){
    WiFiClient client;                      // Wi-Fiクライアントの定義
    int i,len,t=0,size=0,ret=0;             // 変数i,j,t=待ち時間,size=保存容量
    char c,to[33],s[257];                   // 文字変数、to=アクセス先,s=汎用
    char *cp,*cp2;
    int headF=0;                            // ヘッダフラグ(0:HEAD 1:EOL 2:DATA)
    unsigned long time;                     // 時間測定用

    memset(out,0,out_len+1);
    if(city<0 || city>100) return 0;
    snprintf(s,128,"rss.weather.yahoo.co.jp/rss/days/%d.xml",city);
    cp=strchr(s,'/');                       // URL内の区切りを検索
    len=(int)(cp-s);
    if( len<1 || len>32) return 0;
    strncpy(to,s,len);                      // 区切り文字までがホスト名
    to[len]='\0';
    strncpy(s,cp,32);                       // 区切り文字以降はファイル名(なお、s < cp)
    Serial.print("HTTP host : ");
    Serial.println(to);
    Serial.print("Filename  : ");
    Serial.println(s);
    Serial.println("Recieving contents...");
    i=0; while( !client.connect(to,80) ){   // 外部サイトへ接続を実行する
        i++; if(i>=3){                      // 失敗時のリトライ処理
            Serial.println("ERROR: Failed to connect");
            return 0;
        }
        Serial.println("WARN: Retrying");
        delay(10);                          // 10msのリトライ待ち時間
    }
    client.print("GET ");                   // HTTP GETコマンドを送信
    client.print(s);                        // 相手先ディレクトリを指定
    client.println(" HTTP/1.0");            // HTTPプロトコル
    client.println("User-Agent: esp-wroom-32");
    client.print("Host: ");                 // ホスト名を指定
    client.print(to);                       // 相手先ホスト名
    client.println();                       // ホスト名の指定を終了
//  client.println("Accept: *\/*");
    client.println("Accept: application/xml");
    client.println("Connection: close");    // セッションの都度切断を指定
    client.println();
    
    time=millis(); s[0]='\0';
    while(t<TIMEOUT){
        if(client.available()){             // クライアントからのデータを確認
            t=0;
            c=client.read();                // TCPデータの読み取り
            if(headF==2 && c){
                s[(size%128)+128]=c;
                size++;
                len = 128 - (size % 128);
                if(len >0 && len < 128 ) i=client.read((uint8_t *)(s+(size%128)+128),len);
                else i=0;
                size += i;
                if( size % 128 ) continue;
                cp=strstr(s,"<item><title>");   // 13文字のキーワード
                if( !cp || (int)(cp-s) >= 128 ){
                    memcpy(s,s+128,128);    // 変数sの後半を前半へ移動
                    memset(s+128,0,129);    // 変数sの後半だけ初期化
                    continue;
                }
                
                // 解析処理
                // <item><title>【_22日（日）_熊本（熊本）_】_曇り_-_20℃/17℃_-_Yahoo!天気・災害
                cp2=strchr(cp,' ');         // 次の区切りスペース文字
                if( !cp2 ){
                    Serial.print("ERROR : parser [day] / ");
                    Serial.println(cp+1);
                    break;
                }
                _ht_data.day=atoi(cp2+1);
                Serial.print("      day : ");
                Serial.println(_ht_data.day);
                cp=strchr(cp2+1,' ');       // 次の区切りスペース文字
                if( !cp ) break;
                cp2=strchr(cp+1,' ');       // 次の区切りスペース文字
                if( !cp2 ) break;
                cp=strchr(cp2+1,' ');       // 次の区切りスペース文字
                if( !cp ){
                    Serial.print("ERROR : parser [Area] / ");
                    Serial.println(cp2+1);
                    break;
                }
                cp2=strchr(cp+1,' ');       // 次の区切りスペース文字
                len=(int)(cp2-cp-1);
                if(len<0|| len>16){
                    Serial.print("ERROR : parser [Weather] / ");
                    Serial.println(cp+1);
                    break;
                }
                strncpy(_ht_data.text,cp+1,len);
                strcpy(_ht_data.disp,"Unknown");
                for(i=0;i<len-2;i++){
                    if( strncmp(_ht_data.text+i,"晴",3) == 0 ){
                        strcpy(_ht_data.disp,"Fine");
                        _ht_data.code=3;
                        break;
                    }
                    if( strncmp(_ht_data.text+i,"曇",3) == 0 ){
                        strcpy(_ht_data.disp,"Cloudy");
                        _ht_data.code=2;
                        break;
                    }
                    if( strncmp(_ht_data.text+i,"雨",3) == 0 ){
                        strcpy(_ht_data.disp,"Rainy");
                        _ht_data.code=1;
                        break;
                    }
                    if( strncmp(_ht_data.text+i,"雪",3) == 0 ){
                        strcpy(_ht_data.disp,"Snowy");
                        _ht_data.code=4;
                        break;
                    }
                }
                Serial.print("  weather : ");
                Serial.print(_ht_data.disp);
                Serial.print(" / ");
                Serial.println(_ht_data.text);
                
                cp=strchr(cp2+1,' ');       // 次の区切りスペース文字
                if( !cp ){
                    Serial.print("ERROR : parser [temp_H] / ");
                    Serial.println(cp2+1);
                    break;
                }
                _ht_data.tempH=atoi(cp+1);
                Serial.print("   temp_H : ");
                Serial.println(_ht_data.tempH);
                cp2=strchr(cp+1,'/');       // 次の区切りスペース文字
                if( !cp ){
                    Serial.print("ERROR : parser [temp_L] / ");
                    Serial.println(cp+1);
                    break;
                }
                _ht_data.tempL=atoi(cp2+1);
                Serial.print("   temp_L : ");
                Serial.println(_ht_data.tempL);
                ret=httpGetBufferedWeather(out, out_len, data_number);
                break;
            }
            #ifdef DEBUG_HTTPG
                Serial.print(c);            // ヘッダ部のシリアル出力表示
            #endif
            if(headF==1){                   // 前回が行端の時
                if(c=='\n') headF=2;        // 今回も行端ならヘッダ終了
                else if(c!='\r') headF=0;
                continue;
            }
            if(c=='\n') headF=1;            // 行端ならフラグを変更
            continue;
        }
        t++;
        delay(1);
    }
    client.stop();                          // クライアントの切断
    Serial.print(size);                     // 保存したファイルサイズを表示
    Serial.print(" Bytes, ");
    Serial.print(millis()-time);
    Serial.println("ms, Done");
    #ifdef DEBUG_HTTPG
        Serial.print(ret);
        Serial.print(" Bytes \"");
        Serial.print(out);
        Serial.println("\"");
    #endif
    return httpGetBufferedWeatherCode();
}
*/

int httpGetWeather(int city, char *out, int out_len){
    return httpGetWeather(city, out, out_len, -1);
}

int httpGetWeather(int city){
    char s[17];
    return httpGetWeather(city, s, 17, -1);
}
