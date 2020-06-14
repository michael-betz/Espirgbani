#!/usr/bin/env python3
"""
From
https://github.com/gddc/ttfquery/blob/master/ttfquery/describe.py
and
http://www.starrhorne.com/2012/01/18/how-to-extract-font-names-from-ttf-files-using-python-and-our-old-friend-the-command-line.html
ported to Python 3
"""

import sys
from fontTools import ttLib

NAME_ID = 4  # font family name
PLATFORM_ID = 3  # Windows platform (that's what bmfont64.exe expects)


def shortName(font):
    """Get the short name from the font's names table"""
    for r in font['name'].names:
        if r.nameID == NAME_ID and r.platformID == PLATFORM_ID:
            return r.toStr()
    return ""


tt = ttLib.TTFont(sys.argv[1])

# screw you `13/5Atom Sans Regular`
print(shortName(tt).replace('/', '\/'))
