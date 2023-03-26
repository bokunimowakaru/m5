# m5/atom_s3
IoT Code Examples for M5 ATOM S3 (AtomS3)

![M5 AtomS3 ワイヤレスLチカ実験](https://bokunimo.net/blog/wp-content/uploads/2023/03/ex01_02_2.jpg)

## M5Stack製 ATOM S3 の 注意点

- ライブラリ M5AtomS3 by M5Stack が必要 (version=0.0.2で動作確認)  
- ライブラリ FastLED が必要  
- setBrightness が無効化されている (version=0.0.2)  
- 色指定による描画において表示色が指定と異なる  
   (ソフト起因の可能性が高い)
- シリアル出力は USBSerial を使用する  
- 内蔵 LED 出力は M5.dis を使って 24bit で色を指定  
    M5.dis.drawpix(0xff0000);
    M5.dis.show();
- LCD へのテキスト文字表示で、左端のドットがかける  
- LCD が、やや斜めになっている  
   (真っすぐに戻してもしても、同じ方向に戻る)  

## 基礎編 サンプルプログラム集 基礎編

フォルダ [m5/atom_s3](https://github.com/bokunimowakaru/m5/tree/master/atom_s3) には、下記のサンプル・プログラムを収録しています。

|フォルダ名   |内容                                                                               |
|-------------|-----------------------------------------------------------------------------------|
|ex00_hello   |Arduino IDE インストール後の動作確認用プログラム                                   |
|ex01_led     |LED制御用プログラム。HTTPサーバ機能によりブラウザから制御可能                      |
|ex02_sw      |押しボタンの送信プログラム。ex01_ledのLEDの制御やLINEへの送信が可能                |
|ex03_lum     |照度センサの送信プログラム。照度値をクラウド(Ambient)に送信しグラフ化が可能        |
|ex05_hum     |温度＋湿度センサの送信プログラム。家じゅうの部屋に設置すれば居住環境の監視が可能   |

## 書き込み方法

M5 ATOM S3 をパソコンのUSB端子に接続し、電源ボタン（本体のサイドボタン）を長押しし（約2秒）、本体内のLEDが緑色に点灯したら、ボタンを離してください。  
また、Arduino IDE の［ツール］メニュー内の［ポート］から、M5 ATOM を接続したシリアルポートを選択してから、書き込みを実行します。  
Arduino IDE のバージョン 2.0.4 の場合、書き込む都度に、シリアルポートを再設定する必要があります。

## GPIO
- GPIO4 赤外線LED 
- GPIO41 ボタン
- ATOM-HAT使用時のI2C：SDAは GPIO6、SCLは GPIO5

## ブログ記事

本サンプル集の概要を筆者のブログページに書きました。動作の様子の写真などを公開しているので、ご覧ください。  

- [M5 AtomS3 (M5Stack製) 用 Arduino サンプル・プログラム](https://bokunimo.net/blog/esp/3464/)  

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
