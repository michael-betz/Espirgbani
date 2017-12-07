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
    tag = b"\x01\x02\x03\x04\x05"
    a = b"".join( dat.flatten()[:128*32//2][0::2] )
    b = b"".join( dat.flatten()[:128*32//2][1::2] )
    ind = a.find( tag )
    if ind >= 0:
        return 2*ind
    ind = b.find( tag )
    if ind >= 0:
        return 2*ind+1
    return ind

def getNextFname():
    global fNameOffset
    fName = b"".join( rawDat[fNameOffset:fNameOffset+32] ).replace(b"\x00",b"").decode("ascii")
    fNameOffset += 0x200
    return fName

with open(fName,"rb") as f:
    rawDat = array( bytearray(f.read()), dtype=uint8 )

xSize = 128
ySize =  32
frameSize = xSize * ySize // 2
offset = 0x12C400
fNameOffset = 0xC814
imlist = []
while(1):
    aniDat = rawDat[offset:offset+frameSize]
    if len(aniDat) < frameSize:
        print("done")
        break
    fOffs = isFooter( aniDat )
    if fOffs >= 0:
        # end of current file, save it
        fName = "./gif/{}.gif".format( getNextFname() )
        print( hex(offset), fName )
        imlist[0].save( fName, save_all=True, append_images=imlist[1:] )
        #start a new file
        imlist = []
        offset += fOffs + 0x200
    else:
        imgData = getFrame(aniDat, xSize, ySize) * 16
        imlist.append( Image.fromarray(imgData, mode="L") )
        offset += frameSize    