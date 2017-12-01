#!/bin/bash

#Simple and stupid script to (re)generate image data. Needs an Unix-ish environment with
#ImageMagick and xxd installed.

convert lenna.png lenna.rgb

OUTF="../anim.c"

echo '//Auto-generated' > $OUTF
echo 'static const unsigned char myanim[]={' >> $OUTF
{
	cat lenna.rgb
} | xxd -i >> $OUTF
echo "};" >> $OUTF
echo 'const unsigned char *anim=&myanim[0];' >> $OUTF
