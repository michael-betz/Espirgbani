#!/bin/bash
shopt -s nullglob
cp ./fonts/*.ttf ~/.fonts
fc-cache -v ~/.fonts
COUNTER=0
for FILE_NAME in ./fonts/*.ttf
do
	FILE_NAME=$(basename "$FILE_NAME")
	# PREFIX=${FILE_NAME%.*}	#Remove file ending
	# PREFIX=${PREFIX// /_}	#Replace spaces by _
	FONT_NAME=$(./fontname.py "./fonts/$FILE_NAME")
	echo ""
	echo "------------"
	echo "$FILE_NAME   --  $COUNTER"
	# Inject filename into ../clockFont.bmfc settings file
	sed -i "s/fontName=.*/fontName=$FONT_NAME/;s/fontFile=.*/fontFile=.\/fonts\/$FILE_NAME/;" ./clockFont.bmfc
	wine bmfont64.exe -c ./clockFont.bmfc -o "./fonts/out/${COUNTER}.fnt"
	mogrify -format bmp "./fonts/out/${COUNTER}_0.png"
	rm "./fonts/out/${COUNTER}_0.png"
	COUNTER=$((COUNTER+1))
done