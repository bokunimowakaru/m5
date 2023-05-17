#!/usr/bin/env python3
# coding: utf-8

################################################################################
# bmp_serv WSGI 版
#
#                                          Copyright (c) 2019-2023 Wataru KUNINO
################################################################################
# 元：https://github.com/bokunimowakaru/iot/blob/master/server/web_serv.py

from wsgiref.simple_server import make_server
Res_Html = [('Content-type', 'text/html; charset=utf-8')]
Res_Text = [('Content-type', 'text/plain; charset=utf-8')]
Res_Png  = [('Content-type', 'image/png')]
Res_Jpeg = [('Content-type', 'image/jpeg')]

jpg_page = 1    # 写真_ページ番号
jpg_page_n = 3  # 合計ページ数
bmp_page = 1    # 写真_ページ番号
bmp_page_n = 3  # 合計ページ数

def wsgi_app(environ, start_response):              # HTTPアクセス受信時の処理
    path  = environ.get('PATH_INFO')                # リクエスト先のパスを代入
    query = environ.get('QUERY_STRING')             # クエリを代入

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
        head += Res_Png                             # PNG形式での応答を設定

    if path == '/photo.jpg':                        # リクエスト先がphoto.jpg
        fp = open('html/photo' + format(jpg_page,'#02d') + '.jpg', 'rb')
        res = fp.read()                             # 画像データを変数へ代入
        fp.close()                                  # ファイルを閉じる
        head += Res_Jpeg                            # JPG形式での応答を設定
        jpg_page += 1
        if jpg_page > jpg_page_n:
            jpg_page = 1

    if path == '/color.bmp':                        # リクエスト先がcolor.bmp
        fp = open('html/color' + format(bmp_page,'#02d') + '.bmp', 'rb')
        res = fp.read()                             # 画像データを変数へ代入
        fp.close()                                  # ファイルを閉じる
        head += Res_Jpeg                            # BMP形式での応答を設定
        bmp_page += 1
        if bmp_page > bmp_page_n:
            bmp_page = 1

    if path[0:7] == '/photo0' and path[-4:] == '.jpg': # リクエスト先がphoto0X.jpg
        fp = open('html'+path, 'rb')                # 画像ファイルを開く
        res = fp.read()                             # 画像データを変数へ代入
        fp.close()                                  # ファイルを閉じる
        head += Res_Jpeg                            # JPG形式での応答を設定

    if path == '/' or path[0:7] == '/index.':       # リクエスト先がルート
        fp = open('html/index.html', 'r')           # HTMLファイルを開く
        res = fp.read().encode()                    # HTML本文を変数へ代入
        fp.close()                                  # ファイルを閉じる
        head += Res_Html                            # TXT形式での応答を設定

    if res is not None:                             # 変数res
        head.append( ('Content-Length',str(len(res))) )  # コンテンツ長
        print(head)
        start_response('200 OK', Res_Text)          # TXT形式での応答を設定
        return [res]                                # 応答メッセージを返却
    else:
        res = 'Not Found\r\n'.encode()
        start_response('404 Not Found', Res_Text)
        return [res]                                # 応答メッセージを返却

try:
    httpd = make_server('', 80, wsgi_app)           # ポート80でHTTPサーバ実体化
    print("HTTP port 80")                           # 成功時にポート番号を表示
except PermissionError:                             # 例外処理発生時に
    httpd = make_server('', 8080, wsgi_app)         # ポート8080でサーバ実体化
    print("HTTP port 8080")                         # 起動ポート番号の表示
httpd.serve_forever()                               # HTTPサーバを起動
