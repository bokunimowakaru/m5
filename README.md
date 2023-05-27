# m5
IoT Code Examples for ESP32, M5Stack, M5Stick C, M5Stick C Plus, ATOM, ATOM  Lite, ATOM S3

## サンプルプログラム集

ESP32開発ボード ESP32-DevKitC, モジュール ESP32-WROOM-32, M5Stack, M5Stick C, M5Stick C Plus, ATOM Lite, ATOM S3 に対応したサンプル・プログラム集です。

## 本コンテンツの最新版とダウンロード方法

    ZIPファイルでのダウンロード
    - https://github.com/bokunimowakaru/m5/zipball/master

    Gitコマンドでのダウンロード方法
    - git clone https://bokunimo.net/git/m5/

    最新版の保存先
    - https://bokunimo.net/git/m5/

![実行画面ex00～07](/pictures/ex00_07_thumb.gif)

## 主なフォルダ名、プログラム名

本レポジトリに収録した主なプログラムのフォルダ名、ファイル名の一覧を示します。

|フォルダ名 |内容                                                  |
|-----------|------------------------------------------------------|
|atom       |M5Stack製 ATOM / ATOM Lite / 通常の ESP32-WROOM-32 用 |
|atom_s3    |M5Stack製 ATOM S3 用
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

Wi-Fiを使用するサンプル・プログラムについては、実行前に #define SSID と #define PASS の書き換えが必要です。
お持ちの無線アクセスポイントに合わせて設定してください。  
m5フォルダ内にある setWifiSSID.sh を使用すれば、一括で全プログラムのSSIDとパスワードを変更できます。  

#### 赤外線リモコン ex08_ir_out

下図は、赤外線リモコン信号を受信したり、LAN内の別の端末のブラウザからの遠隔制御でリモコン信号を送信することが出来る、ex08_ir_outの画面の一例です。  

![実行画面ex08](/pictures/ex08_thumb.gif)

詳細説明（M5Stackでリモコン送信&受信）
[https://bokunimo.net/blog/esp/2685/](https://bokunimo.net/blog/esp/2685/)  
M5Stack with IR Remote Unit receives infrared signal or transmits infrared signal from your PCs in the LAN.

#### 音声合成 ex09_talk

下図は、音声合成 AquesTalk Pico LSI ATP3012 で音声を発話する ex09_talk の動作例です。LAN内の端末のブラウザからローマ字ベースの音声コードを入力して、しゃべらせることも可能です。  

![実行画面ex09](/pictures/ex09_thumb.gif)

詳細説明（M5Stackで音声出力）
[https://bokunimo.net/blog/esp/2708/](https://bokunimo.net/blog/esp/2708/)  

#### カメラ ex10_cam

下図は、M5Stackに接続したカメラ (SeeedStudio Grove Serial Camera Kit)を、LAN内の端末のブラウザから撮影し、液晶画面とブラウザに表示するプログラム ex10_cam の実行例です。  

![実行画面ex10](/pictures/ex10_thumb.gif)

詳細説明（M5Stackで防犯カメラ）
[https://bokunimo.net/blog/esp/2722/](https://bokunimo.net/blog/esp/2722/)  

### 応用編

|フォルダ名   |内容                                                                               |
|-------------|-----------------------------------------------------------------------------------|
|ex11_ble_scan|Bluetooth LE のアドバタイジング送信数を数えるBLEビーコン・センサ                   |
|ex12_janken  |Webインタフェース HTTP GET でクラウド・サーバとジャンケン対決します                |
|ex13_daruma  |人感センサ・ユニット（PIR Motion Sensor）を使った だるまさんがころんだ ゲーム      |
|ex14_mogura  |インターネット上でランキング競争 M5Stackのボタンを使った もぐらたたき ゲーム       |
|ex15_clock   |インターネット時刻を取得してアナログ時計風に表示＆アラーム時刻にLINEに通知         |
|ex16_weather |インターネットから天気予報情報を取得して天気アイコンで表示                         |
|ex17_signage |デジタル・サイネージ for M5Stack, LAN or インターネットから情報を取得して表示      |

![実行画面ex11～14](/pictures/ex11_14_thumb.gif)

#### 時計 ex15_clock

アナログ時計風の画面を表示するインターネット対応の時計です。アラーム時刻になるとLINEに通知し、スマホなどでアラームが鳴ったことを確認することが出来ます。  
下図は、左から順に、(1)起動時、(2)インターネット時刻取得後、(3)アラーム設定画面、(4)アラーム待機画面です。  

![実行画面ex15](/pictures/ex15_clock_thumb_1.png)

時計画面は、上記を含めて5パターンを準備しました。  

![実行画面ex15](/pictures/ex15_clock_thumb_2.png)

詳細説明（M5StackでLINEにアラーム通知するIoT時計）
[https://bokunimo.net/blog/esp/2773/](https://bokunimo.net/blog/esp/2773/)  

#### 天気予報情報表示 ex16_weather

天気予報情報を表示する IoT TeleTele坊主 for M5Stackです。1時間ごとに気象庁の天気予報情報サイトにアクセスし、アナログ時計風のM5Stackの画面上にアイコン表示します。降水確率や予想気温も表示します。  
なお、本ソフトを利用した M5Stack を不特定多数の人が見れる用途で使用すると、気象業務に該当し、法律違反となる場合があります（ソースコード内の参考情報を参照）。  
![実行画面ex16](/pictures/ex16_weather_thumb.png)

詳細説明（M5Stackで天気アイコン表示）
[https://bokunimo.net/blog/esp/3426/](https://bokunimo.net/blog/esp/3426/)  

#### デジタル・サイネージ for M5Stack ex17_signage

定期的にHTTPサーバから画像を取得し、LCDに表示します。  
左ボタンでJPEG画像、中央ボタンでBMP画像、右ボタンで時計表示なしのBMP画像を表示します。  
HTTPサーバは、Raspberry Piなどで動作するHTTPサーバ(toolsフォルダ内に保存したsignage_serv.py)を用います。  
このように役割を分担することで、Raspberry Pi側の様々なコンテンツをM5Stack上で表示できるようになります。
本例のsignage_serv.pyでは、M5Stackからのアクセスを受けるたびに、異なる画像を配信し、スライドショーのように表示することが出来ます。  
さらに、Raspberry Pi側のプログラムで配信するコンテンツを動的に作ることも可能です。
その場合も、M5Stack側はHTTPサーバから取得したJPEG画像やBMP画像を表示するだけなので、ソフトウェアの書き換え不要で、新しいコンテンツに対応することが出来ます。  
※現在、M5Stack CORE2版のみを公開しています。

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
- ブログ [https://bokunimo.net/blog/](https://bokunimo.net/blog/)
- M5Stackメニュー [https://bokunimo.net/blog/menu/m5stack/](https://bokunimo.net/blog/menu/m5stack/)

----------------------------------------------------------------

## GitHub Pages  

*  (This Document)  
  [https://git.bokunimo.com/m5/](https://git.bokunimo.com/m5/)  

* M5 ATOM S3  
  [https://git.bokunimo.com/m5/atom_s3/](https://git.bokunimo.com/m5/atom_s3/)

----------------------------------------------------------------

# git.bokunimo.com GitHub Pages site
[http://git.bokunimo.com/](http://git.bokunimo.com/)  

----------------------------------------------------------------
