#!/usr/bin/python3
"""
	convert extracted runDMD image file to a series of .gif files
	usage:
		dmd2gif.py ./dmd.img ./outFolder
"""
import sys
from numpy import array, zeros, uint8
from PIL import Image

if len(sys.argv) != 3:
	print(__doc__)
	sys.exit()

fName = sys.argv[1]
outFolder = sys.argv[2]

def getFrame( buf, xSize=128, ySize=32, frameOffset=0, byteOffset=None ):
    """ returns unpacked image data, 1 byte per pixel """
    if byteOffset is None:
        byteOffset = xSize*ySize*frameOffset
    rawDat = buf[byteOffset:byteOffset+ySize*xSize//2]
    # unpack 2 pixels / byte to 1 pixel / byte
    a = zeros(xSize*ySize, dtype=uint8)
    a[1::2] = rawDat & 0x0F
    a[0::2] = rawDat >> 4
    return a.reshape((ySize,xSize))

def isFooter( dat ):
    """ returns >= 0 if a footer frame of size 0x200 bytes is detected """
    # we search for this sequence: xx 01 xx 02 xx 03 xx 04 xx
    tag = b"\x01\x02\x03\x04"
    a = b"".join( dat.flatten()[:frameSize][0::2] )
    b = b"".join( dat.flatten()[:frameSize][1::2] )
    ind = a.find( tag )
    if ind >= 0:
        return 2*ind
    ind = b.find( tag )
    if ind >= 0:
        return 2*ind+1
    return ind

def getNextFname():
    """ get next file name from the header table """
    global fNameOffset
    fName = b"".join( rawDat[fNameOffset:fNameOffset+32] ).replace(b"\x00",b"").decode("ascii")
    fNameOffset += 0x200
    return fName

with open(fName,"rb") as f:
    rawDat = array( bytearray(f.read()), dtype=uint8 )

xSize = 128
ySize =  32
frameSize   = xSize * ySize // 2   # each data byte holds two 4 bit pixels
offset      = 0x12C400             # Start of first animation
fNameOffset = 0x00C814             # Start of first filename
loopOn = {
    "24_002": 5,
    "THE_CHAMPION_PUB_002": 5,
    "THE_CHAMPION_PUB_003": 5,
    "THE_CHAMPION_PUB_004": 5,
    "THE_CHAMPION_PUB_007": 3,
    "THE_CHAMPION_PUB_015": 3,
    "THE_CHAMPION_PUB_018": 5,
    "THE_CHAMPION_PUB_025": 5,
    "THE_CHAMPION_PUB_032": 2,
    "THE_CHAMPION_PUB_042": 3,
    "THE_CHAMPION_PUB_044": 5,
    "THE_CHAMPION_PUB_045": 5,
    "THE_CHAMPION_PUB_046": 5,
    "THE_CHAMPION_PUB_049": 5,
    "THE_CHAMPION_PUB_050": 7,
    "THE_CHAMPION_PUB_067": 5,
    "THE_CHAMPION_PUB_068": 5,
}
imlist = []
aniStartOffset = offset
aniLengthBytes = 0
with open("offsets.txt","w") as f:
    while(1):
        aniDat = rawDat[offset:offset+frameSize]
        if len(aniDat) < frameSize:
            print("done")
            break
        fOffs = isFooter( aniDat )
        if fOffs >= 0:
            # end of current file, save it
            fName = getNextFname()
            fullFName = "./gif/{}.gif".format( fName )
            print( hex(offset), fullFName )
            f.write("0x{:08x},0x{:08x}\n".format(aniStartOffset, aniLengthBytes) )
            if fName in loopOn:
                for im in imlist:
                    im.info["loop"] = loopOn[fName]
            imlist[0].save( fullFName, "GIF", optimize=False, save_all=True, append_images=imlist[1:] )
            #start a new file
            imlist = []
            offset += fOffs + 0x200
            aniStartOffset = offset
            aniLengthBytes = 0
        else:
            imgData = getFrame(aniDat, xSize, ySize) * 16
            img = Image.fromarray(imgData, mode="L")
            img.info["background"]   = 0x00*16
            img.info["transparency"] = 0x0A*16
            img.info["duration"]     = 1/10 * 1000
            imlist.append( img )
            aniLengthBytes += frameSize
            offset += frameSize    