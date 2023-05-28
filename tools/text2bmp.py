#!/usr/bin/env python3
# coding: utf-8

################################################################################
# text2bmp.py
# テキスト文字を2値のBMP形式に変換します。
# def main()関数内の変数textにテキスト文字を代入して、実行してください。
# htmlフォルダ内に out.bmp が生成されます。
# さざなみフォントを使用する場合は、参考文献からダウンロードしてください。
#
#                                          Copyright (c) 2023 Wataru KUNINO
################################################################################
# 参考文献は末尾に示しています。

from PIL import Image, ImageDraw, ImageFont
from unicodedata import east_asian_width
from sys import argv
from sys import stdin
from os.path import isfile
PIX_WHITE = True

def text2bmp(text, font_size=12):
    if len(text) <= 0:
        return None
    image = Image.new('1', (320, 240))                  # mode '1' for 1-bit color
    draw = ImageDraw.Draw(image)
    if font_size == 8:
        font = ImageFont.truetype('font/misaki_gothic.ttf', 8, encoding='unic')
        dx = 4
        dy = 8
    else:
        font = ImageFont.truetype('font/sazanami-gothic.ttf', 12, encoding='unic')
        dx = 6
        dy = 12
    draw.rectangle((0,0,320,240), outline=0, fill = (not PIX_WHITE)*255)
    i = 0
    x = 0
    for y in range(240//dy):
        while x < (320//dx):
            draw.text((x*dx,y*dy), text[i], font=font, fill=PIX_WHITE*255)
            if east_asian_width(text[i]) in 'FWA':
                x += 1
            i += 1
            x += 1
            if i >= len(text):
                break
            if (x >= (320//dx)-1) and (east_asian_width(text[i]) in 'FWA'):
                break
            if text[i] == '\r':
                i += 1
            if text[i] == '\n':
                i += 1
                break
        x = 0
        if i >= len(text):
           break
    print(text[0:i])
    image.save("html/out.bmp")
    return image

def main():                                             # メイン関数
    text = "人間失格　太宰治（出展：青空文庫）　はしがき\n　私は、その男の写真を三葉、見たことがある。\n　一葉は、その男の、幼年時代、とでも言うべきであろうか、十歳前後かと推定される頃の写真であって、その子供が大勢の女のひとに取りかこまれ、（それは、その子供の姉たち、妹たち、それから、従姉妹たちかと想像される）庭園の池のほとりに、荒い縞の袴をはいて立ち、首を三十度ほど左に傾け、醜く笑っている写真である。醜く？　けれども、鈍い人たち（つまり、美醜などに関心を持たぬ人たち）は、面白くも何とも無いような顔をして、\n「可愛い坊ちゃんですね」\n　といい加減なお世辞を言っても、まんざら空お世辞に聞えないくらいの、謂わば通俗の「可愛らしさ」みたいな影もその子供の笑顔に無いわけではないのだが、しかし、いささかでも、美醜に就いての訓練を経て来たひとなら、ひとめ見てすぐ、\n　「なんて、いやな子供だ」\n　と頗る不快そうに呟き、毛虫でも払いのける時のような手つきで、その写真をほうり投げるかも知れない。\n　まったく、その子供の笑顔は、よく見れば見るほど、何とも知れず、イヤな薄気味悪いものが感ぜられて来る。どだい、それは、笑顔でない。この子は、少しも笑ってはいないのだ。その証拠には、この子は、両方のこぶしを固く握って立っている。人間は、こぶしを固く握りながら笑えるものでは無いのである。猿だ。猿の笑顔だ。ただ、顔に醜い皺を寄せているだけなのである。「皺くちゃ坊ちゃん」とでも言いたくなるくらいの、まことに奇妙な、そうして、どこかけがらわしく、へんにひとをムカムカさせる表情の写真であった。私はこれまで、こんな不思議な表情の子供を見た事が、いちども無かった。\n　第二葉の写真の顔は、これはまた、びっくりするくらいひどく変貌していた。学生の姿である。高等学校時代の写真か、大学時代の写真か、はっきりしないけれども、とにかく、おそろしく美貌の学生である。しかし、これもまた、不思議にも、生きている人間の感じはしなかった。学生服を着て、胸のポケットから白いハンケチを覗かせ、籐椅子に腰かけて足を組み、そうして、やはり、笑っている。こんどの笑顔は、皺くちゃの猿の笑いでなく、かなり巧みな微笑になってはいるが、しかし、人間の笑いと、どこやら違う。血の重さ、とでも言おうか、生命の渋さ、とでも言おうか、そのような充実感は少しも無く、それこそ、鳥のようではなく、羽毛のように軽く、ただ白紙一枚、そうして、笑っている。つまり、一から十まで造り物の感じなのである。キザと言っても足りない。軽薄と言っても足りない。ニヤケと言っても足りない。おしゃれと言っても、もちろん足りない。しかも、よく見ていると、やはりこの美貌の学生にも、どこか怪談じみた気味悪いものが感ぜられて来るのである。私はこれまで、こんな不思議な美貌の青年を見た事が、いちども無かった。"
    text2bmp(text,8)
    return

if __name__ == "__main__":                              # プログラム実行時に
    argc = len(argv)                                    # 引数の数をargcへ代入
    print('Usage: '+argv[0]+' [-stdin]')                # タイトル表示
    if argc >=2 and argv[1] == '-stdin':
        text =''
        for line in stdin:
            text += line
        if isfile('font/sazanami-gothic.ttf'):
            text2bmp(text,12)
        else:
            text2bmp(text,8)
    else:
        main()                                          # メイン関数を実行
    exit()                                              # プログラムの終了

################################################################################
# 参考文献 Raspberry Pi 3で128×64ピクセルのOLEDに日本語を表示するサンプル
# https://gist.github.com/yuasatakayuki/d1aaf166a5e3a2e9fa8ecf6990b01ac7
################################################################################
# 参考文献 さざなみフォント efont
# https://ja.osdn.net/projects/efont/
################################################################################
# 参考文献 青空文庫 （インターネットの電子図書館）
# https://www.aozora.gr.jp/
################################################################################
# 参考文献 NHKニュース （NHKオンライン テキスト版）
# https://k.nhk.jp/knews/new.html

