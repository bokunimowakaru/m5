#!/usr/bin/env python3
# coding: utf-8

################################################################################
# signage_serv.py
# ex17_signage デジタル・サイネージ for M5Stack にコンテンツを配信するHTTPサーバ
# ・一般ユーザ(piユーザなど)で使用するとポート番号8080でHTTPサーバが起動します。
# ・インターネット・ブラウザなどでアクセスするとコンテンツを応答します。
# ・サーバ上で動作確認する場合は http://127.0.0.1:8080/ にアクセスしてください。
#
#                                          Copyright (c) 2019-2023 Wataru KUNINO
################################################################################
# 参考文献(引用元)
# https://github.com/bokunimowakaru/iot/blob/master/server/web_serv.py
################################################################################
# テスト方法の一例
# http://127.0.0.1:8080/                            (index.htmlを表示)
# http://127.0.0.1:8080/image.png                   (PNG画像ファイルを表示)
# http://127.0.0.1:8080/photo.jpg                   (JPEG画像ファイルを表示)
# http://127.0.0.1:8080/mono.bmp                    (2値BMPファイルを表示)
# http://127.0.0.1:8080/image.png?x=320&y=240       (320x240に変換して表示)
# http://127.0.0.1:8080/photo.jpg?x=128&y=128       (128x128に変換して表示)
# http://127.0.0.1:8080/photo.jpg?x=0&y=0           (元のサイズで表示)

from wsgiref.simple_server import make_server
from urllib import parse
from os.path import isfile
from PIL import Image
import io

Res_Html = [('Content-type', 'text/html; charset=utf-8')]
Res_Text = [('Content-type', 'text/plain; charset=utf-8')]
Res_Png  = [('Content-type', 'image/png')]
Res_Jpeg = [('Content-type', 'image/jpeg')]
Res_Bmp  = [('Content-type', 'image/bmp')]

jpg_page = 0    # JPEG画像_ページ番号, 0はphoto.jpg
jpg_page_n = 3  # JPEG画像_最大ページ番号(photo01.jpg～photo03.jpg)
bmp_page = 0    # BMP画像_ページ番号, 0はmono.bmp
bmp_page_n = 3  # BMP画像_最大ページ番号(mono01.bmp～mono03.bmp)

def resize(data,x,y,format='JPEG'):
    fp = io.BytesIO(data)                           # がぞプデータをBytesIO に
    image = Image.open(fp)                          # PILのオブジェクトにロード
    del fp                                          # 解放
    fp = io.BytesIO()                               # 空のBytesIOを生成
    if format == 'BMP':
        image.resize((x,y)).convert('1').save(fp,'BMP') # 2値BMPに変換
    else:
        image.resize((x,y)).convert('RGB').save(fp,format)
    return fp.getvalue()                            # BytesIOデータを応答

def wsgi_app(environ, start_response):              # HTTPアクセス受信時の処理
    disp_x = 0                                      # 配信用のコンテンツ幅指定
    disp_y = 0                                      # 配信用のコンテンツ高さ指定
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

    if path == '/ok.txt':                           # リクエスト先がok.txtの時
        res = 'OK\r\n'.encode()                     # 応答メッセージ作成
        head += Res_Text                            # TXT形式での応答を設定

    if path == '/ok.html':                          # リクエスト先がok.htmlの時
        res = '<html><h3>OK</h3></html>'.encode()   # 応答メッセージ作成
        head += Res_Html                            # HTML形式での応答を設定

    if path == '/image.png':                        # リクエスト先がimage.png
        fp = open('html/image.png', 'rb')           # 画像ファイルを開く
        res = fp.read()                             # 画像データを変数へ代入
        fp.close()                                  # ファイルを閉じる
        if disp_x > 0 and disp_y > 0:               # ファイルサイズの変更処理
            res = resize(res,disp_x,disp_y,'PNG')   # 変換
        head += Res_Png                             # PNG形式での応答を設定

    if path == '/photo.jpg':                        # リクエスト先がphoto.jpg
        if jpg_page == 0:
            fp = open('html/photo.jpg', 'rb')
        else:
            fp = open('html/photo' + format(jpg_page,'#02d') + '.jpg', 'rb')
        res = fp.read()                             # 画像データを変数へ代入
        fp.close()                                  # ファイルを閉じる
        if disp_x > 0 and disp_y > 0:               # ファイルサイズの変更処理
            res = resize(res,disp_x,disp_y,'JPEG')  # 変換
        head += Res_Jpeg                            # JPG形式での応答を設定
        jpg_page += 1
        if jpg_page > jpg_page_n:
            jpg_page = 0

    if path[0:7] == '/photo0' and path[-4:] == '.jpg': # リクエスト先がphoto0X.jpg
        fp = open('html'+path, 'rb')                # 画像ファイルを開く
        res = fp.read()                             # 画像データを変数へ代入
        fp.close()                                  # ファイルを閉じる
        if disp_x > 0 and disp_y > 0:               # ファイルサイズの変更処理
            res = resize(res,disp_x,disp_y,'JPEG')  # 変換
        head += Res_Jpeg                            # JPG形式での応答を設定

    if path == '/mono.bmp':                         # リクエスト先がmono.bmp
        if isfile('html/out.bmp'):
            fp = open('html/out.bmp', 'rb')
        elif bmp_page == 0:
            fp = open('html/mono.bmp', 'rb')
        else:
            fp = open('html/mono' + format(bmp_page,'#02d') + '.bmp', 'rb')
        res = fp.read()                             # 画像データを変数へ代入
        fp.close()                                  # ファイルを閉じる
        if disp_x > 0 and disp_y > 0:               # ファイルサイズの変更処理
            res = resize(res,disp_x,disp_y,'BMP')   # 変換
        head += Res_Bmp                             # BMP形式での応答を設定
        bmp_page += 1
        if bmp_page > bmp_page_n:
            bmp_page = 0

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
