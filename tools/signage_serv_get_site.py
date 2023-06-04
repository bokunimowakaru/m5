#!/usr/bin/env python3
# coding: utf-8

################################################################################
# signage_serv_get_site.py
# ex17_signage_get_site デジタル・サイネージ for M5Stack にWebコンテンツを配信
#
# 実行方法：
# $ ./signage_serv_get_site.py ⏎
#
# 詳細：
# https://bokunimo.net/blog/esp/3698/

#                                          Copyright (c) 2019-2023 Wataru KUNINO
################################################################################
# 参考文献1: Raspberry Pi でブラウザを自動操作してみる 【Python】, いろはぷらっと
# https://irohaplat.com/raspberry-pi-selenium-installation/
################################################################################
# 参考文献2: Webサイトのスクリーンショットを自動化する方法, Kazuki Yonemoto
# https://zenn.dev/kazuki_tam/articles/6c3cf0729c5b847cc2a4
################################################################################
# 参考文献3: Selenium 公式サイト
# https://www.selenium.dev/
################################################################################
# 参考文献(引用元)
# https://github.com/bokunimowakaru/iot/blob/master/server/web_serv.py
################################################################################

url = 'https://ambidata.io/bd/board.html?id=128'    # コンテンツのURL例(Ambient)

# コンテンツの例 (気象庁-大阪府の天気予報，NICT-日本標準時，bokunimo.net-ホーム)
# url='https://www.jma.go.jp/bosai/forecast/#area_type=offices&area_code=270000'
# url='https://www.nict.go.jp/JST/JST5.html'
# url='https://bokunimo.net/'

from PIL import Image
import io

from selenium import webdriver                      # Selenium インポート(文献2)
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC

from wsgiref.simple_server import make_server       # Webサーバ インポート
from urllib import parse

Res_Html = [('Content-type', 'text/html; charset=utf-8')]
Res_Text = [('Content-type', 'text/plain; charset=utf-8')]
Res_Png  = [('Content-type', 'image/png')]
Res_Jpeg = [('Content-type', 'image/jpeg')]
Res_Bmp  = [('Content-type', 'image/bmp')]

windowSizeWidth = 640                               # ブラウザのショット幅
windowSizeHeight = 480                              # ブラウザのショット高さ指定
disp_x = 320                                        # 配信用のコンテンツ幅指定
disp_y = 240                                        # 配信用のコンテンツ高さ指定

options = webdriver.ChromeOptions()                 # Chromeドライバ設定
options.add_argument('--headless=new')              # ヘッドレスモード(文献2)
options.add_argument('--no-sandbox')
options.add_argument('--disable-dev-shm-usage')
driver = webdriver.Chrome('chromedriver',options=options)
driver.implicitly_wait(10)
driver.get(url)                                     # サイトURL取得
WebDriverWait(driver, 15).until(EC.presence_of_all_elements_located)
driver.set_window_size(windowSizeWidth, windowSizeHeight)

def wsgi_app(environ, start_response):              # HTTPアクセス受信時の処理
    global disp_x, disp_y
    path  = environ.get('PATH_INFO')                # リクエスト先のパスを代入
    # print(path)
    query = parse.parse_qsl(environ.get('QUERY_STRING'))  # クエリを代入
    try:
        for (key, val) in query:
            if key == 'x':
                disp_x = int(val)
            if key == 'y':
                disp_y = int(val)
    except ValueError:
        print('ERROR, ValueError, query =',query)

    res = None                                      # 応答値を代入する変数の定義
    head = []
    global jpg_page, jpg_page_n, bmp_page, bmp_page_n

    if path == '/image.png':                        # リクエスト先がimage.png
        driver.save_screenshot('html/chrome.png')   # スクリーン格納(文献2)
        '''
        fp = open('html/chrome.png', 'rb')          # 画像ファイルを開く
        res = fp.read()                             # 画像データを変数へ代入
        fp.close()                                  # ファイルを閉じる
        '''
        png = driver.get_screenshot_as_png()        # Chromeからキャプチャ
        fs = io.BytesIO(png)                        # BytesIO に転送
        res = fs.getvalue()                         # BytesIO からresに代入
        head += Res_Png                             # PNG形式での応答を設定

    if path == '/photo.jpg':                        # リクエスト先がphoto.jpg
        png = driver.get_screenshot_as_png()        # Chromeからキャプチャ
        fs = io.BytesIO(png)                        # BytesIO に転送
        image = Image.open(fs)                      # PILのオブジェクトにロード
        del fs                                      # 解放
        fs = io.BytesIO()                           # 空のBytesIOを生成
        image.resize((disp_x, disp_y)).convert('RGB').save(fs, format='JPEG')
        res = fs.getvalue()                         # resに代入 #↑JPEG変換
        head += Res_Jpeg                            # JPG形式での応答を設定

    if path == '/mono.bmp':                         # リクエスト先がmono.bmp
        png = driver.get_screenshot_as_png()        # Chromeからキャプチャ
        fs = io.BytesIO(png)                        # BytesIO に転送
        image = Image.open(fs)                      # PILのオブジェクトにロード
        del fs                                      # 解放
        fs = io.BytesIO()                           # 空のBytesIOを生成
        image.resize((disp_x, disp_y)).convert('1').save(fs, format='BMP')
        res = fs.getvalue()                         # resに代入 #↑BMP変換
        head += Res_Bmp                             # BMP形式での応答を設定

    if path == '/' or path[0:7] == '/index.':       # リクエスト先がルート
        fp = open('html/index.html', 'r')           # HTMLファイルを開く
        res = fp.read().encode()                    # HTML本文を変数へ代入
        fp.close()                                  # ファイルを閉じる
        head += Res_Html                            # TXT形式での応答を設定

    if res is not None:                             # 変数res
        head.append( ('Content-Length',str(len(res))) )  # コンテンツ長
        # print(head)
        start_response('200 OK', head)              # 応答を設定
        return [res]                                # 応答メッセージを返却
    else:
        res = 'Not Found\r\n'.encode()
        start_response('404 Not Found', Res_Text)
        return [res]                                # 応答メッセージを返却

def main():                                         # メイン関数
    try:
        httpd = make_server('', 80, wsgi_app)       # ポート80でHTTPサーバ実体化
        print("HTTP port 80")                       # 成功時にポート番号を表示
    except PermissionError:                         # 例外処理発生時に
        httpd = make_server('', 8080, wsgi_app)     # ポート8080でサーバ実体化
        print("HTTP port 8080")                     # 起動ポート番号の表示
    while True:
        httpd.serve_forever()                       # HTTPサーバを起動

if __name__ == "__main__":                          # プログラム実行時に
    main()                                          # メイン関数を実行
    driver.quit()                                   # ブラウザ稼働終了(文献2)
