#!/bin/bash
shopt -s nullglob
for FILE_NAME in *.ttf
do
	PREFIX=${FILE_NAME%.*}
	FONT_NAME=$(../fontname.py "$FILE_NAME")
	sed -i "s/fontName=.*/fontName=$FONT_NAME/;s/fontFile=.*/fontFile=fonts\/$FILE_NAME/;" ../clockFont.bmfc
	wine ../bmfont64.exe -c ../clockFont.bmfc -o out/$PREFIX.fnt
	mogrify -format bmp ./out/${PREFIX}_0.png
	rm ./out/${PREFIX}_0.png
done