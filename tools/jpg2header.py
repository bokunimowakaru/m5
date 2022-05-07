#!/usr/bin/env python3
# coding: utf-8

################################################################################
# Jpegファイルをヘッダファイルに変換します
#
################################################################################
# 使用、改変、再配布は自由に行えますが、無保証です。権利情報の改変は不可です。
# Copyright (c) 2020-2021 Wataru KUNINO
# 
# 引用元：
# https://github.com/bokunimowakaru/m5camera/blob/master/tools/gz2header.py
################################################################################

filename = ''
n = 0
from time import sleep
from sys import argv					# 本プログラムの引数argvを取得する
print(argv[0])							# プログラム名を表示する

while len(argv) >= 2:
	filename = argv[1]
	del argv[1]
	n = filename.find('.jpg')
	if n > 0:
		saveto = filename[0:n] + '_jpg.h'
	else:
		print('ERROR', filename)
		continue

	print('input filename  =',filename)
	print('output filename =',saveto)
	fp = open(filename, mode='br')
	data_array = fp.read()
	fp.close()
	out = []
	for d in data_array:
		out.append(d)

	fp = open(saveto, mode='w')
	print('#define '+filename[0:n]+'_jpg_len',len(out), file = fp)
	print('const uint8_t '+filename[0:n]+'_jpg[] = {', file = fp)
	i=0
	for d in out:
		# print(' ' + hex(d), end='', file = fp)
		print(' 0x' + format(d, '02X'), end='', file = fp)
		i += 1
		if i % 16 == 0:
			print(',', file = fp)
		else:
			if i < len(out):
				print(',', end='', file = fp)
	print('\n};', file = fp)
	fp.close()
