# m5
IoT Code Examples for ESP32, M5Stack, M5Stick C, M5Stick C Plus, ATOM, ATOM  Lite

## サンプル集

ESP32開発ボード ESP32-DevKitC, モジュール ESP32-WROOM-32, M5Stack, M5Stick C, M5Stick C Plus, ATOM Lite に対応したサンプル・プログラム集です。

## 本コンテンツの最新版とダウンロード方法

    最新版の保存先
    - https://bokunimo.net/git/m5/

    ダウンロード方法(GitHubから)
    - git clone https://bokunimo.net/git/m5/

![実行画面ex00～07](/pictures/ex00_07_thumb.gif)

## 主なフォルダ名、プログラム名

本レポジトリに収録した主なプログラムのフォルダ名、ファイル名の一覧を示します。

|フォルダ名 |内容                                                  |
|-----------|------------------------------------------------------|
|atom       |M5Stack製 ATOM / ATOM Lite / 通常の ESP32-WROOM-32 用 |
|core       |M5Stack製 Core 用                                     |
|stick_cplus|M5Stack製 M5Stick C Plus 用                           |
|pictures   |関連画像ファイル                                      |
|tools      |関連ツール                                            |
|LICENSE    |ライセンス内容(MITライセンス:要権利表示・無保証)      |

### 基礎編

|フォルダ名   |内容                                                                               |
|-------------|-----------------------------------------------------------------------------------|
|ex00_hello   |Arduino IDE インストール後の動作確認用プログラム                                   |
|ex01_led     |LED制御用プログラム。HTTPサーバ機能によりブラウザから制御可能                      |
|ex02_sw      |押しボタンの送信プログラム。ex01_ledのLEDの制御やLINEへの送信が可能                |
|ex03_lum     |照度センサの送信プログラム。照度値をクラウド(Ambient)に送信しグラフ化が可能        |
|ex04_lcd     |小型液晶への表示プログラム。ex02、03、05の送信データの表示が可能                   |
|ex05_hum     |温度＋湿度センサの送信プログラム。家じゅうの部屋に設置すれば居住環境の監視が可能   |
|ex06_pir     |人感センサ・ユニット（PIR Motion Sensor）を使ったWi-Fi人感センサ用プログラム       |
|ex07_gps     |GNSS/GPS位置情報を送信する位置情報送信プログラム                                   |
|ex08_ir_out  |赤外線リモコン・ユニット（IR Unit）を使ったWi-Fi赤外線・リモコン用プログラム       |
|ex09_talk    |Wi-Fiコンシェルジェ［音声アナウンス担当］音声合成 AquesTalk Pico LSI ATP3012用     |
|ex10_cam     |Wi-Fiコンシェルジェ［カメラ担当］SeeedStudio Grove Serial Camera Kit 用            |

下図は、赤外線リモコン信号を受信したり、LAN内の別の端末のブラウザからの遠隔制御でリモコン信号を送信することが出来る、ex08_ir_outの画面の一例です。  

![実行画面ex08](/pictures/ex08_thumb.gif)

### 応用編

|フォルダ名   |内容                                                                               |
|-------------|-----------------------------------------------------------------------------------|
|ex11_ble_scan|Bluetooth LE のアドバタイジング送信数を数えるBLEビーコン・センサ                   |
|ex12_janken  |Webインタフェース HTTP GET でクラウド・サーバとジャンケン対決します                |
|ex13_daruma  |人感センサ・ユニット（PIR Motion Sensor）を使った だるまさんがころんだ ゲーム      |
|ex14_mogura  |インターネット上でランキング競争 M5Stackのボタンを使った もぐらたたき ゲーム       |

![実行画面ex11～14](/pictures/ex11_14_thumb.gif)

## Arduino IDE 用の ESP32 開発環境のセットアップ

ESP32開発ボード（ ESP32-WROOM-32 搭載）で使用する場合、下記の手順で開発環境をセットアップし、
「atom」フォルダ内のサンプルを使用してください。

	atom フォルダ : ESP32開発ボード ESP32-WROOM-32 対応サンプル

1. Arduino IDE (https://www.arduino.cc/en/software/) をインストールする。
2. Arduino IDE を起動し、[ファイル]メニュー内の[環境設定]を開き、「追加のボードマネージャのURL」の欄に下記の「安定版」を追加する。

    安定版
    - https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

    開発途上版
    - https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_dev_index.json

    参考文献
    - https://github.com/espressif/arduino-esp32 (最新情報)
    - https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html (情報が古い場合があるので注意)

3. [ツール]メニュー内の[ボード]からボードマネージャを開き、検索窓に「esp32」を入力後、esp32 by Espressif Systems をインストールする。

4. [ツール]メニュー内の[ボード]で ESP32 Dev Module を選択する。

5. M5Stack Atomの場合は、[ツール]メニュー内の[Upload Speed]で115200を選択する。
その他のM5Stack/M5Stickの場合は、M5Stack社のインストール方法を参照してください。

by 国野 亘 Wataru KUNINO 
- ウェブサイト [https://bokunimo.net/](https://bokunimo.net/)
- ブログ [https://bokuniomo.net/blog/](https://bokuniomo.net/blog/)
- カテゴリESP [https://bokunimo.net/blog/category/esp/](https://bokunimo.net/blog/category/esp/)

----------------------------------------------------------------
# git.bokunimo.com GitHub Pages site
[http://git.bokunimo.com/](http://git.bokunimo.com/)  
----------------------------------------------------------------
