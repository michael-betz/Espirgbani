#!/bin/bash
set -e
shopt -s nullglob

mkdir -p fonts/out

cp ./fonts/*.ttf ~/.fonts
fc-cache -v ~/.fonts

COUNTER=0
for FILE_NAME in fonts/*.ttf
do
	FILE_NAME=$(basename "$FILE_NAME")
	FONT_NAME=$(./fontname.py "./fonts/$FILE_NAME")

	# adjust font size
	FONT_SIZE=-36
	CHAR_SET=0
	if [ "$FONT_NAME" = "000_INSOMNIAC_COMIC_DIALOGUE" ]; then FONT_SIZE=-55; fi
	if [ "$FONT_NAME" = "5Darius  Regular" ]; then FONT_SIZE=-28; fi
	if [ "$FONT_NAME" = "1900.80.5" ]; then FONT_SIZE=-43; fi
	if [ "$FONT_NAME" = "Anime Ace Bold" ]; then FONT_SIZE=-28; fi
	if [ "$FONT_NAME" = "GL-Nummernschild-Mtl" ]; then FONT_SIZE=-45; fi
	if [ "$FONT_NAME" = "Hot Pizza" ]; then FONT_SIZE=-38; fi
	if [ "$FONT_NAME" = "Digital-7" ]; then FONT_SIZE=-45; fi
	if [ "$FONT_NAME" = "DK Crayon Crumble" ]; then FONT_SIZE=-45; fi
	if [ "$FONT_NAME" = "Xiomara" ]; then FONT_SIZE=-47; fi
	if [ "$FONT_NAME" = "M0N0T3CHN0L0GY2407 Regular" ]; then FONT_SIZE=-50; fi
	if [ "$FONT_NAME" = "Stoehr numbers" ]; then FONT_SIZE=-32; fi
	if [ "$FONT_NAME" = "13\/5Atom Sans Regular" ]; then FONT_SIZE=-55; fi
	if [ "$FONT_NAME" = "4114 Blaster" ]; then FONT_SIZE=-43; fi
	if [ "$FONT_NAME" = "EarwigFactory-Regular" ]; then FONT_SIZE=-40; fi
	if [ "$FONT_NAME" = "Runy Tunes Revisited NF" ]; then FONT_SIZE=-35; fi

	if [ "$FONT_NAME" = "October Twilight" ]; then CHAR_SET=238; fi

	echo "$COUNTER: $FILE_NAME, $FONT_NAME, $FONT_SIZE"

	# inject filename into ../clockFont.bmfc settings file
	sed -i "s/fontName=.*/fontName=$FONT_NAME/;s/fontFile=.*/fontFile=.\/fonts\/$FILE_NAME/;" clockFont.bmfc

	# inject font size into .bmfc
	sed -i "s/fontSize=.*/fontSize=$FONT_SIZE/" clockFont.bmfc

	# inject charSet into .bmfc
	sed -i "s/charSet=.*/charSet=$CHAR_SET/" clockFont.bmfc

	F_OUT=$(printf "fonts/out/%02d" $COUNTER)

	# sometimes bmfont returns 1 for unknown reasons
	set +e
	wine bmfont64.exe -c clockFont.bmfc -o "${F_OUT}.fnt"
	set -e

	mogrify -format bmp "${F_OUT}_0.png"

	rm "${F_OUT}_0.png"

	COUNTER=$((COUNTER+1))
done
