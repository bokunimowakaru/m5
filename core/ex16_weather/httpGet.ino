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

#define TIMEOUT 6000                        // タイムアウト 6秒
#include <WiFi.h>                           // ESP32用WiFiライブラリ
#include <WiFiClientSecure.h>               // TLS(SSL)通信用ライブラリ
#include <HTTPClient.h>                     // HTTP通信用ライブラリ

#define BUF_N 128                           // バッファサイズ 2の倍数 128以上

int _httpg_day=-1;                           // 天気予報日
char _httpg_weather[17]="";                 // 天気予報
char _httpg_weather_disp[9]="";             // 天気予報・表示用(Fine/Cloudy/Rainy/Snowy)
char _httpg_weather_code=0;                 // 天気コード(4:Snowy,3:Fine,2:Cloudy,1:Rainy)
int _httpg_temp_H=0;                        // 天気予想温度（最高気温）
int _httpg_temp_L=0;                        // 天気予想温度（最低気温）

int httpGetBufferedWeather(char *out, int out_len, int data_number){
    if(_httpg_day == 0) return 0;
    memset(out,0,out_len+1);
    switch(data_number){
        case 0:
            snprintf(out,out_len+1,"%d,%d,%d,%s,%s",_httpg_day,_httpg_temp_H,_httpg_temp_L,_httpg_weather_disp,_httpg_weather);
            break;
        case 1:
            snprintf(out,out_len+1,"%d",_httpg_day);
            break;
        case 2:
            snprintf(out,out_len+1,"%d",_httpg_temp_H);
            break;
        case 3:
            snprintf(out,out_len+1,"%d",_httpg_temp_L);
            break;
        case 4:
            snprintf(out,out_len+1,"%s",_httpg_weather_disp);
            break;
        case 5:
            snprintf(out,out_len+1,"%s",_httpg_weather);
            break;
        default:
            snprintf(out,out_len+1,"%s %d/%d",_httpg_weather_disp,_httpg_temp_H,_httpg_temp_L);
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
    return _httpg_weather_code;
}

int httpGetBufferedTemp(){
    return (_httpg_temp_L + _httpg_temp_H)/2;
}

int httpGetBufferedTempH(){
    return _httpg_temp_H;
}

int httpGetBufferedTempL(){
    return _httpg_temp_L;
}

int httpGetWeather(int city, char *out, int out_len, int data_number){
    int i,dist,len,ret=0,size,done;
    char c,s[BUF_N+1];                        // 文字変数、s=汎用
    char *cp,*cp2;
    unsigned long time=millis();            // 時間測定用

    memset(out,0,out_len);
    memset(_httpg_weather,0,17);
    memset(_httpg_weather_disp,0,9);
    s[BUF_N]=0;
    if(city<0 || city>=1000000) return 0;
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
        return 0;
    }


//  https.getString().toCharArray(s, 2048);     // 受信結果を変数sへ代入
//  size=strlen(s);
    WiFiClient * stream = https.getStreamPtr(); // get tcp stream
    size = stream->available();
    len -= stream->readBytes(s+BUF_N/2, ((size > BUF_N/2) ? BUF_N/2 : size));
    done = 0;

    // 解析処理 weathers
    while(https.connected() && len > 0 && size > 0){
        memcpy(s,s+BUF_N/2,BUF_N/2);
        len -= stream->readBytes(s+BUF_N/2, ((size > BUF_N/2) ? BUF_N/2 : size));
        cp = s;
        while(cp - s < BUF_N/2){
            if(done == 0){
                cp = strstr(cp,"\"weathers\"");
                if(!cp || cp - s >= BUF_N/2) break;
                cp += 13;
                dist=0;
                _httpg_weather_code=0;
                cp2 = strstr(cp, "晴れ");
                if(cp2){
                    dist = (int)(cp2 - cp);
                    strcpy(_httpg_weather, "晴");
                    strcpy(_httpg_weather_disp,"Fine");
                    _httpg_weather_code=3;
                }
                cp2 = strstr(cp, "くもり");
                if(cp2 && (int)(cp2 - cp) < dist){
                    dist = (int)(cp2 - cp);
                    strcpy(_httpg_weather, "曇");
                    strcpy(_httpg_weather_disp,"Cloudy");
                    _httpg_weather_code=2;
                }
                cp2 = strstr(cp, "雨");
                if(cp2 && (int)(cp2 - cp) < dist){
                    dist = (int)(cp2 - cp);
                    strcpy(_httpg_weather, "雨");
                    strcpy(_httpg_weather_disp,"Rainy");
                    _httpg_weather_code=1;
                }
                cp2 = strstr(cp, "雪");
                if(cp2 && (int)(cp2 - cp) < dist){
                    dist = (int)(cp2 - cp);
                    strcpy(_httpg_weather, "雪");
                    strcpy(_httpg_weather_disp,"Snowy");
                    _httpg_weather_code=4;
                }
                if(_httpg_weather_code==0){
                    strcpy(_httpg_weather, "－");
                    strcpy(_httpg_weather_disp,"Unknown");
                }
                Serial.print("  weather : ");
                Serial.print(_httpg_weather_disp);
                Serial.print(" / ");
                Serial.println(_httpg_weather);
                done++;
            }
            if(done == 1){
                cp = strstr(cp,"\"tempsMin\"");
                if(!cp || cp - s >= BUF_N/2) break;
                cp += 16;
                _httpg_temp_L=atoi(cp);
                Serial.print("   temp_L : ");
                Serial.println(_httpg_temp_L);
                done++;
            }
            if(done == 2){
                cp = strstr(s,"\"tempsMax\"");
                if(!cp || cp - s >= BUF_N/2) break;
                cp += 16;
                _httpg_temp_H=atoi(cp);
                Serial.print("   temp_H : ");
                Serial.println(_httpg_temp_H);
                done++;
            }
            if(done == 3) break;
        }
        if(done == 3) break;
        size = stream->available();
        delay(1);
    }
    https.end();                                // HTTPクライアントの処理を終了
    client.stop();                              // TLS(SSL)通信の停止
    delay(1);
    
    Serial.print(millis()-time);
    Serial.println("ms, Done");
    
    httpGetBufferedWeather(out, out_len, data_number);
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
                _httpg_day=atoi(cp2+1);
                Serial.print("      day : ");
                Serial.println(_httpg_day);
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
                strncpy(_httpg_weather,cp+1,len);
                strcpy(_httpg_weather_disp,"Unknown");
                for(i=0;i<len-2;i++){
                    if( strncmp(_httpg_weather+i,"晴",3) == 0 ){
                        strcpy(_httpg_weather_disp,"Fine");
                        _httpg_weather_code=3;
                        break;
                    }
                    if( strncmp(_httpg_weather+i,"曇",3) == 0 ){
                        strcpy(_httpg_weather_disp,"Cloudy");
                        _httpg_weather_code=2;
                        break;
                    }
                    if( strncmp(_httpg_weather+i,"雨",3) == 0 ){
                        strcpy(_httpg_weather_disp,"Rainy");
                        _httpg_weather_code=1;
                        break;
                    }
                    if( strncmp(_httpg_weather+i,"雪",3) == 0 ){
                        strcpy(_httpg_weather_disp,"Snowy");
                        _httpg_weather_code=4;
                        break;
                    }
                }
                Serial.print("  weather : ");
                Serial.print(_httpg_weather_disp);
                Serial.print(" / ");
                Serial.println(_httpg_weather);
                
                cp=strchr(cp2+1,' ');       // 次の区切りスペース文字
                if( !cp ){
                    Serial.print("ERROR : parser [temp_H] / ");
                    Serial.println(cp2+1);
                    break;
                }
                _httpg_temp_H=atoi(cp+1);
                Serial.print("   temp_H : ");
                Serial.println(_httpg_temp_H);
                cp2=strchr(cp+1,'/');       // 次の区切りスペース文字
                if( !cp ){
                    Serial.print("ERROR : parser [temp_L] / ");
                    Serial.println(cp+1);
                    break;
                }
                _httpg_temp_L=atoi(cp2+1);
                Serial.print("   temp_L : ");
                Serial.println(_httpg_temp_L);
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
