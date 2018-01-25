#!/usr/bin/env python

"""
From
https://github.com/gddc/ttfquery/blob/master/ttfquery/describe.py
and
http://www.starrhorne.com/2012/01/18/how-to-extract-font-names-from-ttf-files-using-python-and-our-old-friend-the-command-line.html
ported to Python 3
"""

import sys
from fontTools import ttLib

FONT_SPECIFIER_NAME_ID = 4

def shortName( font ):
    """Get the short name from the font's names table"""
    for record in font['name'].names:
        if record.nameID == FONT_SPECIFIER_NAME_ID:
            return record.toStr()
    return ""

tt = ttLib.TTFont(sys.argv[1])
print( shortName(tt) )