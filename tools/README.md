-------------------------------------------------------------------------------
# [udp_monitor_chart.py](https://github.com/bokunimowakaru/m5/blob/master/tools/udp_monitor_chart.py)

UDPで受信したIoTセンサ機器の値を棒グラフで表示します。  

![実行例](https://git.bokunimo.com/m5/pictures/ex03_lum_site.png)  

下記は、さくらのクラウドを使った時のパケットフィルタの設定例です。  
無保証です。  

![パケットフィルタ](https://git.bokunimo.com/m5/tools/%E3%82%B3%E3%83%A9%E3%83%A0_%E3%83%91%E3%82%B1%E3%83%83%E3%83%88%E3%83%95%E3%82%A3%E3%83%AB%E3%82%BF_http_8080.png)  
https://git.bokunimo.com/m5/tools/コラム_パケットフィルタ_http_8080.png

## 参考文献
https://manual.sakura.ad.jp/cloud/network/packet-filter.html#id12  
https://knowledge.sakura.ad.jp/7133/  

-------------------------------------------------------------------------------
# [jpg2header.py](https://github.com/bokunimowakaru/m5/blob/master/tools/jpg2header.py)

JPEGファイルをヘッダファイルに変換します  

-------------------------------------------------------------------------------
# [bmp2header.py](https://github.com/bokunimowakaru/m5/blob/master/tools/bmp2header.py)

BMPファイルをヘッダファイルに変換します  

-------------------------------------------------------------------------------
# [signage_serv.py](https://github.com/bokunimowakaru/m5/blob/master/tools/signage_serv.py)

ex17_signage デジタル・サイネージ for M5Stack にコンテンツを配信するHTTPサーバ
・一般ユーザ(piユーザなど)で使用するとポート番号8080でHTTPサーバが起動します。
・インターネット・ブラウザなどでアクセスするとコンテンツを応答します。
・サーバ上で動作確認する場合は http://127.0.0.1:8080/ にアクセスしてください。

-------------------------------------------------------------------------------
# [text2bmp.py](https://github.com/bokunimowakaru/m5/blob/master/tools/text2bmp.py)

テキスト文字を2値のBMP形式に変換します。  
def main()関数内の変数textにテキスト文字を代入して、実行してください。  
htmlフォルダ内に out.bmp が生成されます。  
さざなみフォントを使用する場合は、参考文献からダウンロードしてください。  

-------------------------------------------------------------------------------
# [get_photo.py](https://github.com/bokunimowakaru/m5/blob/master/tools/get_photo.py)

Wi-Fiコンシェルジェ［カメラ担当］ex10_cam で製作したカメラ上で動作するHTTPサーバにアクセスし、Raspberry Pi上に保存するHTTPクライアントです。  
本ソフトを起動してから、5分以内にカメラを起動すると、カメラのIPアドレスを受信し、当該カメラによる撮影を待ち受けます。  
ex10_cam による撮影(通知)時と、Wi-Fi人感センサ ex06_pir が人体などの動きを検知した時に、写真を撮影し、photoフォルダに写真を保存します。

-------------------------------------------------------------------------------
by 国野 亘 Wataru KUNINO  

- ウェブサイト bokunimo.net [https://bokunimo.net/](https://bokunimo.net/)
- ブログ [https://bokuniomo.net/blog/](https://bokuniomo.net/blog/)
- M5Stackのブログ [https://bokunimo.net/blog/menu/m5stack/](https://bokunimo.net/blog/menu/m5stack/)

----------------------------------------------------------------

## GitHub Pages  

*  m5  
  [https://git.bokunimo.com/m5/](https://git.bokunimo.com/m5/)  

*  (This Document)  
  [https://git.bokunimo.com/m5/tools/](https://git.bokunimo.com/m5/tools/)  

----------------------------------------------------------------

# git.bokunimo.com GitHub Pages site
[http://git.bokunimo.com/](http://git.bokunimo.com/)  

----------------------------------------------------------------
