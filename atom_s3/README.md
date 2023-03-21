## M5Atom S3の独自の注意点

- ライブラリ M5AtomS3 by M5Stack が必要 (version=0.0.2で動作確認)
- ライブラリ FastLED が必要
- setBrightness が無効化されている (version=0.0.2)
- 表示色が不正確  
   (保有ハードウェアの問題か? 白と黒は出ているのでソフト起因の可能性が高い)
- シリアル出力は USBSerial を使用する
- 内蔵 LED 出力は M5.dis を使って 24bit で色を指定
    M5.dis.drawpix(0xff0000);
    M5.dis.show();

## 書き込み方法
If you need to burn the firmware, please press and hold the reset button (about 2 seconds) until the internal green LED lights up, then you can release it, at this time the device has entered download mode and waited for burning.  
また、Arduino IDE (2.0.4で確認)のシリアルポートを、書き込む都度に設定する必要がある。

## GPIO
- GPIO4 赤外線LED 
- GPIO41 ボタン
- ATOM-HAT使用時のI2C：SDAは GPIO6、SCLは GPIO5

## 参考文献
- Arduino IDE 開発環境イントール方法：  
  https://docs.m5stack.com/en/quick_start/atoms3/arduino

- M5StickC Arduino Library API 情報 (M5StackC 用 ※ATOM S3用ではない )：  
  https://docs.m5stack.com/en/api/stickc/system_m5stickc

- AtomS3 Library (on GitHub)：  
  https://github.com/m5stack/M5AtomS3

- ピン配列：  
  https://static-cdn.m5stack.com/resource/docs/products/core/AtomS3/img-b85e925c-adff-445d-994c-45987dc97a44.jpg

## Specification
    MCU                     ESP32-S3FN8
    DCDC                    SY8089
    IMU                     MPU6886
    LCD                     N085-1212TBWIG06-C08
    Operating temperature   0°C ~ 40°C
    Resolution              128(H)RGB x 128(V)
    support voltage         5V
    Power supply mode       TYPE C
    output voltage          3.3V
    IO interface × 6       G5/G6/G7/G8/G38/G39
    Screen communication protocol   SPI
    Product Size            24mm × 24mm × 13mm
    Package Size            65mm × 44.5mm × 14mm
    Product Weight          6.8g
    Package Weight          10.9g
    https://docs.m5stack.com/en/core/AtomS3
