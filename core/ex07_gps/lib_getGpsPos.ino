/*******************************************************************************
本ソースは下記からダウンロードしたものを基にして作成しました。(2018/4/14～)
    https://github.com/mikalhart/TinyGPS

元のライセンスは本フォルダの下記にコピーしてあります。
    ./lib_TinyGPS_LICENSE.txt

改変部のライセンスは以下の通りです。
    /LICENSE
    MIT License
    Copyright (c) 2022 Wataru KUNINO
*******************************************************************************/

/*******************************************************************************
TinyGPS
********************************************************************************
A compact Arduino NMEA (GPS) parsing library http://arduiniana.org

Mikal Hart
mikalhart
Block or report user

The Sundial Group
Austin, TX, USA

http://sundial.com
*******************************************************************************/

// #define SOFTWARE_SERIAL      // ソフトウェアによるシリアル接続に使用

#include <Arduino.h>
#ifdef SOFTWARE_SERIAL
    #include "lib_SoftwareSerial.h"
#endif
#include "lib_TinyGPS.h"
#define PIN_GPS_RX 22           // GPS モジュール NEO-6M 【TXピン】
#define PIN_GPS_TX 21           // GPS モジュール NEO-6M 【RXピン】未使用

#ifdef SOFTWARE_SERIAL
    SoftwareSerial ss(PIN_GPS_RX, PIN_GPS_TX);
#else
    HardwareSerial ss(2);       // uart_nr デフォルト RX=16, TX=17
#endif

void setupGps(){
    #ifdef SOFTWARE_SERIAL
        ss.begin(9600);
    #else
        ss.begin(9600, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
    #endif
}

bool getGpsRawData(char *s, int len){
    int i=0;
    #ifdef SOFTWARE_SERIAL
        ss.listen();
    #endif
    s[0] = '\0';
    boolean en = 0;
    for (unsigned long start = millis(); millis() - start < 1000;){
        if(ss.available()){
            char c = ss.read();
            if(c =='$') en = 1; // 開始
            // if(c =='*') en = 0; // 終了
            if(c =='\r' || c =='\n') return 1;
            if(en){
                s[i] = c;
                i++;
                s[i] = '\0';
                if(i >= len - 1){
                    return 0;   // オーバフロー
                }
            }
        }
    }
    return 0;   // タイムアウト
}

bool getGpsPos(TinyGPS &gps, float *flat, float *flon, float *falt){
    unsigned short sat;
    unsigned long hdop;
    unsigned long age;
    return getGpsPos(gps,flat,flon,falt,&sat,&hdop,&age);
}

bool getGpsPos(
    TinyGPS &gps,
    float *flat,
    float *flon,
    float *falt,
    unsigned short *sat,
    unsigned long *hdop,
    unsigned long *age
){
    bool newData = false;
    #ifdef SOFTWARE_SERIAL
        ss.listen();
    #endif
    for (unsigned long start = millis(); millis() - start < 1000;){
        while(ss.available()){
            char c = ss.read();
            if(gps.encode(c)){
                newData = true;
                break;
            }
        }
    }
    if(newData){
        gps.f_get_position(flat, flon, age);
        *falt=gps.f_altitude();
        *hdop=gps.hdop();
        *sat=gps.satellites();
        Serial.print("LAT=");
        Serial.print(*flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : *flat, 6);
        Serial.print(" LON=");
        Serial.print(*flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : *flon, 6);
        Serial.print(" ALT=");
        Serial.print(*falt, 2);
        Serial.print(" SAT=");
        Serial.print(*sat == TinyGPS::GPS_INVALID_SATELLITES ? 0 : *sat);
        Serial.print(" PREC=");
        Serial.print(*hdop == TinyGPS::GPS_INVALID_HDOP ? 0 : *hdop);
        Serial.print(" AGE=");
        Serial.print(*age);
    }
    unsigned long chars;
    unsigned short sentences;
    unsigned short failed_cs;
    gps.stats(&chars, &sentences, &failed_cs);
    Serial.print(" CHARS=");
    Serial.print(chars);
    Serial.print(" SENTENCES=");
    Serial.print(sentences);
    Serial.print(" CSUM ERR=");
    Serial.println(failed_cs);
    if (chars == 0) Serial.println("** No characters received from GPS: check wiring **");
    return newData;
}
