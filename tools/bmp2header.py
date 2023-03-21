#!/usr/bin/env python3
# coding: utf-8

################################################################################
# BMPファイルをヘッダファイルに変換します
#
################################################################################
# 使用、改変、再配布は自由に行えますが、無保証です。権利情報の改変は不可です。
# Copyright (c) 2020-2021 Wataru KUNINO
# 
# 引用元：
# https://github.com/bokunimowakaru/m5camera/blob/master/tools/gz2header.py
################################################################################
# 参考文献
# https://algorithm.joho.info/image-processing/bmp-file-data-header/
# https://algorithm.joho.info/programming/python/bitmap-file-header-read/

filename = ''
n = 0
from time import sleep
from sys import argv					# 本プログラムの引数argvを取得する
print(argv[0])							# プログラム名を表示する

while len(argv) >= 2:
	filename = argv[1]
	del argv[1]
	n = filename.find('.bmp')
	if n > 0:
		saveto = filename[0:n] + '_bmp.h'
	else:
		print('ERROR', filename)
		continue

	print('input filename  =',filename)
	print('output filename =',saveto)
	fp = open(filename, mode='br')
	data_array = fp.read()
	fp.close()
	for i in range(54): # ヘッダ処理
		print("0x" + format(data_array[i], '02X'), end=", ")
		if i % 16 == 15:
			print("")
	print("")

	# ヘッダ部の情報を切り分け
	# https://algorithm.joho.info/programming/python/bitmap-file-header-read/
	format_type = data_array[0:2] # 画像フォーマットの種類を取得
	file_size = data_array[2:6] # ファイルサイズを取得
	header_size = data_array[10:14] # ヘッダサイズを取得
	width, height = data_array[18:22], data_array[22:26] # 画像の高さと幅を取得
	color_bit = data_array[28:30] # 1画素の色数を取得  
	
	# リトルエンディアン方式で16進数から10進数に変換
	# https://algorithm.joho.info/programming/python/bitmap-file-header-read/
	file_size = int.from_bytes(file_size, 'little')
	header_size = int.from_bytes(header_size, 'little')
	width = int.from_bytes(width, 'little')
	height = int.from_bytes(height, 'little')  
	color_bit = int.from_bytes(color_bit, 'little') 
	
	# ヘッダ情報をコンソール出力
	# https://algorithm.joho.info/programming/python/bitmap-file-header-read/
	print("Format type  :", format_type.decode())
	print("File size    :", file_size, "[byte]", ", len =",len(data_array))
	print("Header size  :", header_size, "[byte]") 
	print("Width        :", width, "[px]")     
	print("Height       :", height, "[px]")
	print("Total pixels :", width*height)  
	print("Color        :", color_bit, "[bit]")  

	if format_type != b'BM':
		print("ERROR format_type", 'BM', format_type.decode())
		exit()
	if file_size - header_size < width * height * int(color_bit/8):
		print("ERROR file_size",file_size - header_size, width * height * int(color_bit/8))
		exit()
	if len(data_array) < file_size - header_size:
		print("ERROR len(data_array)",len(data_array), file_size - header_size)
		exit()

	out = []
	#	for d in data_array[header_size:]:
	# 		out.append(d)
	for y in range(height):
		for x in range(width):
			p = (height - y - 1) * width + x
			p *= int(color_bit/8)
			p += header_size
			for bit in range(int(color_bit/8)):
				try:
					out.append(data_array[p])
				except Exception as e:
					print(e)
					print("p =",p, ", height =",height, ", width =",width, ", x =",x, ", y =",y)
					exit()
				p += 1

	bmp_len = len(out)
	if color_bit == 16:
		bmp_len = int(bmp_len / 2)
	if color_bit == 24:
		bmp_len = int(bmp_len / 3)

	fp = open(saveto, mode='w')
	print('#define '+filename[0:n]+'_bmp_len',bmp_len, file = fp)
	print('const uint16_t '+filename[0:n]+'_bmp[] = {', file = fp)
	i=0
	rgb565 = 0;
	bit = 0
	for d in out:
		if bit == 0:
			rgb565 = 0
		if color_bit == 8:
			rgb565 = ((d>>3)<<11) | ((d>>2)<<5) | (d>>3)
		if color_bit == 16:
			rgb565 |= d<<(8 * bit)
		if color_bit == 24:
			if bit == 0:
				rgb565 |= ((d>>3)<<11)
			elif bit == 1:
				rgb565 |= ((d>>2)<<5)
			elif bit == 2:
				rgb565 |= (d>>3)
		bit += 1
		if bit * 8 == color_bit:
			bit = 0
			print(' 0x' + format(rgb565, '04X'), end='', file = fp)
			i += 1
			if i % 16 == 0:
				print(',', file = fp)
			else:
				if i < len(out):
					print(',', end='', file = fp)
	print('\n};', file = fp)
	# print('bmp_len =', bmp_len, i)
	fp.close()
	print('Done\n')
