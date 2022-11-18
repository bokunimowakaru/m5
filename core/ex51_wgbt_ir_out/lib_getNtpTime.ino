/*******************************************************************************
NTPクライアント

現在時刻をインターネットから取得してWi-Fi送信する機器です。
あたかも「時刻センサ」のような動作を行います。

本ソースコードの末尾に引用したプログラムのライセンスについて表示しています。

                                          Copyright (c) 2016-2019 Wataru KUNINO

https://github.com/bokunimowakaru/esp/blob/master/2_example/example57_fs/getNTP.ino
*******************************************************************************/

#include <WiFiUdp.h>                        // UDP通信を行うライブラリ

#define NTP_PACKET_SIZE 48                  // NTP時刻長48バイト
#define LEAP_YEAR(Y)     ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )

void time2txt(char *date,unsigned long local){	// char date[20];
// functions to convert to and from system time
    int Year,year;
    int Month,month, monthLength;
    int Day;
    int Second,Minute,Hour,Wday;            // Sunday is day 1 
    unsigned long days;
    static  const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};
    
    Second = local % 60;
    local /= 60; // now it is minutes
    Minute = local % 60;
    local /= 60; // now it is hours
    Hour = local % 24;
    local /= 24; // now it is days
    Wday = ((local + 4) % 7) + 1;           // Sunday is day 1 
    
    year = 0;  
    days = 0;
    while((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= local) {
        year++;
    }                           //  Year = year; // year is offset from 1970 
    days -= LEAP_YEAR(year) ? 366 : 365;
    local  -= days; // now it is days in this year, starting at 0

    days=0;
    month=0;
    monthLength=0;
    for (month=0; month<12; month++) {
        if (month==1) {         // february
            if (LEAP_YEAR(year)) {
                monthLength=29;
            } else {
                monthLength=28;
            }
        } else {
            monthLength = monthDays[month];
        }
        if (local >= monthLength) {
            local -= monthLength;
        } else {
            break;
        }
    }
    Year = year + 1970;
    Month = month + 1;  // jan is month 1  
    Day = local + 1;     // day of month
    
    sprintf(date,"%4d/%02d/%02d,%02d:%02d:%02d",Year,Month,Day,Hour,Minute,Second);
               //  4+1+ 2+1+ 2+1+ 2+1+ 2+1+ 2 +1 = 20 Bytes
}

String time2str(unsigned long local){
    char date[20];
    time2txt(date, local);
    return String(date); 
}

unsigned long getNtpTime(const char* address, const int port){
    byte packetBuffer[NTP_PACKET_SIZE];     // 送受信用バッファ
    WiFiUDP udp;                            // NTP通信用のインスタンスを定義
    unsigned long highWord;                 // 時刻情報の上位2バイト用
    unsigned long lowWord;                  // 時刻情報の下位2バイト用
    unsigned long time;                     // 1970年1月1日からの経過秒数
    int waiting=0;                          // 待ち時間カウント用
    char s[20];                             // 表示用

    Serial.println("getNtpTime");           // 「Example 13」をシリアル出力表示
    udp.begin(port);                        // NTP待ち受け開始
    
    // send an NTP request to the time server at the given address
    memset(packetBuffer,0,NTP_PACKET_SIZE); // set all bytes in the buffer to 0
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011;           // LI, Version, Mode
    packetBuffer[1] = 0;                    // Stratum, or type of clock
    packetBuffer[2] = 6;                    // Polling Interval
    packetBuffer[3] = 0xEC;                 // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12]  = 49;
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52; // all NTP fields have been given values, now
    // send a packet requesting a timestamp //NTP requests are to port 123
    udp.beginPacket(address, 123);
    udp.write(packetBuffer, NTP_PACKET_SIZE);
    udp.endPacket();

    while(udp.parsePacket()<44){
        delay(100);                         // 受信待ち
        waiting++;                          // 待ち時間カウンタを1加算する
        if(waiting%10==0)Serial.print('.'); // 進捗表示
        if(waiting > 100) return 0;         // 100回(10秒)を過ぎたらスリープ
    }
    udp.read(packetBuffer,NTP_PACKET_SIZE); // 受信パケットを変数packetBufferへ
    highWord=word(packetBuffer[40],packetBuffer[41]);   // 時刻情報の上位2バイト
    lowWord =word(packetBuffer[42],packetBuffer[43]);   // 時刻情報の下位2バイト
    
    Serial.print("UTC time = ");
    time = highWord<<16 | lowWord;          // 時刻(1900年1月からの秒数)を代入
    time -= 2208988800UL;                   // 1970年と1900年の差分を減算
    time2txt(s,time);                       // 時刻をテキスト文字に変換
    Serial.println(s);                      // テキスト文字を表示

    Serial.print("JST time = ");            // 日本時刻
    time += 32400UL;                        // +9時間を加算
    time2txt(s,time);                       // 時刻をテキスト文字に変換
    Serial.println(s);                      // テキスト文字を表示
    
    udp.stop();
    return time;
}

/*******************************************************************************
本ソフトウェアには下記のソフトウェアが含まれます。
改変前の製作物の権利は下記のとおりです。
********************************************************************************
 
 Udp NTP Client
 
 Get the time from a Network Time Protocol (NTP) time server
 Demonstrates use of UDP sendPacket and ReceivePacket
 For more on NTP time servers and the messages needed to communicate with them,
 see http://en.wikipedia.org/wiki/Network_Time_Protocol
 
 created 4 Sep 2010
 by Michael Margolis
 modified 9 Apr 2012
 by Tom Igoe
 
 This code is in the public domain.
 
********************************************************************************

 time.c - low level time and date functions
 Copyright (c) Michael Margolis 2009
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 
 6  Jan 2010 - initial release 
 12 Feb 2010 - fixed leap year calculation error
 1  Nov 2010 - fixed setTime bug (thanks to Korman for this)

*******************************************************************************/
