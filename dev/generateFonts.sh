#!/bin/bash
shopt -s nullglob

mkdir ./fonts/out

cp ./fonts/*.ttf ~/.fonts
fc-cache -v ~/.fonts

COUNTER=0
for FILE_NAME in ./fonts/*.ttf
do
	FILE_NAME=$(basename "$FILE_NAME")
	FONT_NAME=$(./fontname.py "./fonts/$FILE_NAME")

	# Inject filename into ../clockFont.bmfc settings file
	sed -i "s/fontName=.*/fontName=$FONT_NAME/;s/fontFile=.*/fontFile=.\/fonts\/$FILE_NAME/;" ./clockFont.bmfc

	# adjust font size
	FONT_SIZE=-36
	if [ "$FILE_NAME" = "5darius_.ttf" ]; then FONT_SIZE=-28; fi
	if [ "$FILE_NAME" = "1900805.ttf" ]; then FONT_SIZE=-43; fi
	if [ "$FILE_NAME" = "animeace_b.ttf" ]; then FONT_SIZE=-28; fi
	if [ "$FILE_NAME" = "GlNummernschildMtl-LrZZ.ttf" ]; then FONT_SIZE=-45; fi
	if [ "$FILE_NAME" = "hotpizza.ttf" ]; then FONT_SIZE=-40; fi
	if [ "$FILE_NAME" = "Kbzipadeedoodah-YG3j.ttf" ]; then FONT_SIZE=-38; fi
	if [ "$FILE_NAME" = "digital-7.ttf" ]; then FONT_SIZE=-45; fi
	if [ "$FILE_NAME" = "DK Crayon Crumble.ttf" ]; then FONT_SIZE=-45; fi
	if [ "$FILE_NAME" = "Xiomara-Script.ttf" ]; then FONT_SIZE=-47; fi
	if [ "$FILE_NAME" = "hotpizza.ttf" ]; then FONT_SIZE=-33; fi
	if [ "$FILE_NAME" = "M0n0t3chn0l0gy2407Regular-yP9e.ttf" ]; then FONT_SIZE=-50; fi

	sed -i "s/fontSize=.*/fontSize=$FONT_SIZE/" ./clockFont.bmfc

	echo "$COUNTER: $FILE_NAME, $FONT_NAME, $FONT_SIZE"

	wine bmfont64.exe -c ./clockFont.bmfc -o "./fonts/out/${COUNTER}.fnt"

	mogrify -format bmp "./fonts/out/${COUNTER}_0.png"

	rm "./fonts/out/${COUNTER}_0.png"

	COUNTER=$((COUNTER+1))
done
