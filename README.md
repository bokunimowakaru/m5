# m5
IoT Code Examples for M5Stack, M5Stick C Plus, ATOM Lite  

## サンプル集


## 本コンテンツの最新版とダウンロード方法  

    最新版の保存先  
    - https://bokunimo.net/git/esp32c3/
    
    ダウンロード方法(GitHubから)
    - git clone https://bokunimo.net/git/esp32c3/

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

|フォルダ名 |内容                                                                               |
|-----------|-----------------------------------------------------------------------------------|
|ex00_hello |Arduino IDE インストール後の動作確認用プログラム                                   |
|ex01_led   |LED制御用プログラム。HTTPサーバ機能によりブラウザから制御可能                      |
|ex02_sw    |押しボタンの送信プログラム。ex01_ledのLEDの制御やLINEへの送信が可能                |
|ex03_lum   |照度センサの送信プログラム。照度値をクラウド(Ambient)に送信しグラフ化が可能        |
|ex04_lcd   |小型液晶への表示プログラム。ex02、03、05の送信データの表示が可能                   |
|ex05_hum   |温度＋湿度センサの送信プログラム。家じゅうの部屋に設置すれば居住環境の監視が可能   |
|ex06_pir   |人感センサ・ユニット（PIR Motion Sensor）を使ったWi-Fi人感センサ用プログラム       |
|ex07_ir_in |赤外線リモコン・ユニット（IR Unit）でリモコンコードを取得するプログラム            |
|ex08_ir_out|赤外線リモコン・ユニット（IR Unit）を使ったWi-Fi赤外線・リモコン用プログラム       |
|ex09_talk  |Wi-Fiコンシェルジェ［音声アナウンス担当］音声合成 AquesTalk Pico LSI ATP3012用     |
|ex10_cam   |Wi-Fiコンシェルジェ［カメラ担当］Grove - Serial Camera Kit用                       |

## Arduino IDE 用の ESP32 開発環境のセットアップ  

ESP32開発ボード（ ESP32-WROOM-32 搭載）で使用する場合、下記の手順で開発環境をセットアップし、
「atom」フォルダ内のサンプルを使用してください。  

	atom フォルダ : ESP32開発ボード ESP32-WROOM-32 対応サンプル  

1. Arduino IDE (https://www.arduino.cc/en/software/) をインストールする。  
2. Arduino IDE を起動し、[ファイル]メニュー内の[環境設定]を開き、「追加のボードマネージャのURL」の欄に下記の「安定板」を追加する。  

    安定板  
    - https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json  

    開発途上版  
    - https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_dev_index.json  

    参考文献  
    - https://github.com/espressif/arduino-esp32 (最新情報)  
    - https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html (情報が古い場合があるので注意)  

3. [ツール]メニュー内の[ボード]からボードマネージャを開き、検索窓に「esp32」を入力後、esp32 by Espressif Systems をインストールする。  

4. [ツール]メニュー内の[ボード]で ESP32C3 DEev Module を選択する。  

by bokunimo.net(https://bokunimo.net/)  
- ブログ (https://bokuniomo.net/blog/)  
- カテゴリESP (https://bokunimo.net/blog/category/esp/)  
