#!/bin/bash

################################################################################
# signage_serv.py ニュース取得 拡張モジュール
#
# ex17_signage デジタル・サイネージ for M5Stack にコンテンツを配信するHTTPサーバ
# signage_serv.py 用の拡張モジュールです。
#
# 実行方法
# nohup ./signage_serv_get_news.sh &> /dev/null &
#
#                                          Copyright (c) 2023 Wataru KUNINO
################################################################################

URL="https://k.nhk.jp/knews/new.html"  # ニュースサイト
INTERVAL=$(( 3 * 60 * 60 ))            # 取得間隔(3時間ごと)

while true; do
	TEXT=`\
		curl https://k.nhk.jp/knews/new.html 2> /dev/null\
			|iconv -f sjis\
			|grep -e "<a href.*html\">･"\
			|sed -e "s/<a href.*html\">･//" -e "s/<\/a><br><br>//"\
			|tr -d '\r'\
	` # ニュースサイトからニュースヘッダのテキストのみを取得する
	#echo "$TEXT"

	TEXT2=`\
		while read line; do\
		    echo -n "◆""$line""　"\
		;done < <(echo "${TEXT}")\
	`
	#echo "$TEXT2"
	echo -e "NHKニュース (https://k.nhk.jp/knews/new.html)\n"${TEXT2} | ./text2bmp.py -stdin
	sleep $INTERVAL
done

################################################################################
# 参考文献 NHKニュース （NHKオンライン テキスト版）
# https://k.nhk.jp/knews/new.html
################################################################################
