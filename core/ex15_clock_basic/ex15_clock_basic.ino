#include <M5Stack.h>                        // M5Stack用ライブラリの組み込み
#include <WiFi.h>                           // ESP32用WiFiライブラリ

#define SSID "1234ABCD"                     // 無線LANアクセスポイントのSSID
#define PASS "password"                     // パスワード
#define NTP_SERVER "ntp.nict.jp"            // NTPサーバのURL
#define NTP_PORT 8888                       // NTP待ち受けポート
#define NTP_INTERVAL 3 * 60 * 60 * 1000     // 3時間

unsigned long TIME = 0;                     // NTPで取得した時刻
unsigned long TIME_ms = 0;                  // NTPにアクセスしたマイコン時間(ms)
unsigned long time_ms = - NTP_INTERVAL;     // Wi-FiをONにしたマイコン時間(ms)

void setup(void) {
    M5.begin();
    clock_init();
    WiFi.mode(WIFI_STA);                    // 無線LANをSTAモードに設定
}

void loop() {
    clock_Needle( (TIME % 43200)*1000 + ((millis() - TIME_ms) % 43200000) );
    if(millis() - time_ms > NTP_INTERVAL){
        time_ms = millis();
        WiFi.begin(SSID,PASS);              // 無線LANアクセスポイント接続
        Serial.println("WIFI begin");
    }
    delay(100);
    if(WiFi.status() != WL_CONNECTED) return;   // Wi-Fi未接続のときに戻る
    M5.Lcd.drawString("Connected",0,0,1);
    TIME = getNtpTime(NTP_SERVER,NTP_PORT); // NTPを用いて時刻を取得
    TIME_ms = millis();
    WiFi.disconnect();                      // Wi-Fiの切断
    M5.Lcd.fillRect(0, 0, 6*9, 8, BLACK);
}
