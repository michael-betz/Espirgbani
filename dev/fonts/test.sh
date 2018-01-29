#!/bin/bash
shopt -s nullglob
COUNTER=0
for FILE_NAME in *.ttf
do
	# PREFIX=${FILE_NAME%.*}	#Remove file ending
	# PREFIX=${PREFIX// /_}	#Replace spaces by _
	FONT_NAME=$(../fontname.py "$FILE_NAME")
	# Inject filename into ../clockFont.bmfc settings file
	sed -i "s/fontName=.*/fontName=$FONT_NAME/;s/fontFile=.*/fontFile=fonts\/$FILE_NAME/;" ../clockFont.bmfc
	wine ../bmfont64.exe -c ../clockFont.bmfc -o "out/${COUNTER}.fnt"
	mogrify -format bmp "./out/${COUNTER}_0.png"
	rm "./out/${COUNTER}_0.png"
	COUNTER=$((COUNTER+1))
done