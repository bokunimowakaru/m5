/*******************************************************************************
本ソースは下記からダウンロード(2018/4/14)したものを基にして作成しました。

https://github.com/mikalhart/TinyGPS

国野亘
********************************************************************************
A compact Arduino NMEA (GPS) parsing library http://arduiniana.org

Mikal Hart
mikalhart
Block or report user

The Sundial Group
Austin, TX, USA

http://sundial.com
*******************************************************************************/

#include <Arduino.h>
#include "lib_SoftwareSerial.h"
#include "lib_TinyGPS.h"
#define PIN_GPS_RX 32           // GPS モジュール NEO-6M 【TXピン】
SoftwareSerial ss(PIN_GPS_RX,2);

void setupGps(){
    ss.begin(9600);
}

bool getGpsPos(TinyGPS &gps, float *flat, float *flon, float *falt){
    bool newData = false;
    unsigned long chars;
    unsigned short sentences, failed;
    ss.listen();
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
        unsigned long age;
        gps.f_get_position(flat, flon, &age);
        *falt=gps.f_altitude();
        Serial.print("LAT=");
        Serial.print(*flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : *flat, 6);
        Serial.print(" LON=");
        Serial.print(*flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : *flon, 6);
        Serial.print(" ALT=");
        Serial.print(*falt, 2);
        Serial.print(" SAT=");
        Serial.print(gps.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : gps.satellites());
        Serial.print(" PREC=");
        Serial.print(gps.hdop() == TinyGPS::GPS_INVALID_HDOP ? 0 : gps.hdop());
        Serial.print(" AGE=");
        Serial.print(age);
    }
    gps.stats(&chars, &sentences, &failed);
    Serial.print(" CHARS=");
    Serial.print(chars);
    Serial.print(" SENTENCES=");
    Serial.print(sentences);
    Serial.print(" CSUM ERR=");
    Serial.println(failed);
    if (chars == 0) Serial.println("** No characters received from GPS: check wiring **");
    return newData;
}
