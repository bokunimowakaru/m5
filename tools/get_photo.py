#!/usr/bin/env python3
# coding: utf-8

################################################################################
# カメラからの配信画像を取得する Python版
#
################################################################################
# 使用、改変、再配布は自由に行えますが、無保証です。権利情報の改変は不可です。
#                                          Copyright (c) 2016-2021 Wataru KUNINO
################################################################################

DEVICE1 = 'cam_a_5'                                     # 配信デバイス名(カメラ)
DEVICE2 = 'pir_s_5'                                     # 配信デバイス名(人感)
SAVETO  = 'photo'                                       # 保存先フォルダ名
IP_CAM  = None                                          # カメラのIPアドレス
PORT    = 1024                                          # UDPポート番号を1024に

import pathlib                                          # ファイル・パス用
import socket                                           # ソケット通信用
import urllib.request                                   # HTTP通信ライブラリ
import datetime                                         # 年月日・時刻管理
import time                                             # シリアル時刻

def cam(ip, filename = 'cam.jpg'):                      # IoTカメラ
    filename = SAVETO + '/' + filename
    url_s = 'http://' + ip                              # アクセス先をurl_sへ
    s = '/cam.jpg'                                      # 文字列変数sにクエリを
    try:
        res = urllib.request.urlopen(url_s + s)         # IoTカメラで撮影を実行
        if res.headers['content-type'].lower().find('image/jpeg') < 0:
            print('Error content-type :',res.headers['content-type'])
            return None                                 # JPEGで無いときにNone
    except urllib.error.URLError:                       # 例外処理発生時
        print('Error urllib :',url_s)                   # エラー表示
        return None                                     # 関数を終了する
    data = res.read()                                   # コンテンツ(JPEG)を読む
    try:
        fp = open(filename, 'wb')                       # 保存用ファイルを開く
    except Exception as e:                              # 例外処理発生時
        print(e)                                        # エラー内容を表示
        return None                                     # 関数を終了する
    fp.write(data)                                      # 写真ファイルを保存する
    fp.close()                                          # ファイルを閉じる
    print('saved file :',filename)                      # 保存ファイルを表示する
    return filename                                     # ファイル名を応答する

print('Get Photo for Python')                           # タイトル表示
pathlib.Path(SAVETO).mkdir(exist_ok=True)               # フォルダ作成
time_start = time.time()                                # 開始時刻シリアル値保持

print('Listening UDP port', PORT, '...')                # ポート番号表示
try:
    sock=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)# ソケットを作成
    sock.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)    # オプション
    sock.bind(('', PORT))                               # ソケットに接続
except Exception as e:                                  # 例外処理発生時
    print(e)                                            # エラー内容を表示
    exit()                                              # プログラムの終了
while sock:                                             # 永遠に繰り返す
    try:
        udp, udp_from = sock.recvfrom(128)              # UDPパケットを取得
        udp = udp.decode()                              # UDPデータを文字列に
    except Exception as e:                              # 例外処理発生時
        print(e)                                        # エラー内容を表示
        continue                                        # whileの先頭に戻る
    s=''                                                # 表示用の文字列変数s
    for c in udp:                                       # UDPパケット内
        if ord(c) > ord(' ') and ord(c) <= ord('~'):    # 表示可能文字
            s += c                                      # 文字列sへ追加
    date = datetime.datetime.today()                    # 日時を取得
    date_s = date.strftime('%Y/%m/%d %H:%M:%S')         # 日時を文字列に変換
    print(date_s + ', ', end='')                        # 日時を出力
    print(udp_from[0], end='')                          # 送信元アドレスを出力
    print(', ' + s, flush=True)                         # 受信データを出力
    if s[5] != '_' or s[7] != ',':                      # 指定書式で無い時
        continue                                        # whileの先頭に戻る
    device = s[0:7]                                     # 先頭7文字をデバイス名
    value = s.split(',')                                # CSVデータを分割
    if device == DEVICE1:                               # カメラから受信
        if (time.time() - time_start < 300):            # 起動後5分以内
            IP_CAM = udp_from[0]                        # カメラIPアドレスを保持
            print('IP_CAM =',IP_CAM)                    # 表示
        elif IP_CAM != udp_from[0]:                     # アドレス不一致時
            print('起動後5分を経過したので送信先は更新しません')
            continue                                    # whileの先頭に戻る
    if device == DEVICE1 or device == DEVICE2:          # 対象機器の時
        if IP_CAM is not None:                          # IP_CAMがNoneでは無い時
            date_f = date.strftime('%Y%m%d-%H%M%S')     # ファイル名用の時刻書式
            cam(IP_CAM,'cam_' + date_f + '.jpg')        # 撮影(ファイル保存)
sock.close()                                            # ソケットの切断
