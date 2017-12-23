#ifndef BMP_H
#define BMP_H

typedef struct {
  uint16_t  bfType;
  uint32_t  bfSize;
  uint16_t  bfReserved1;
  uint16_t  bfReserved2;
  uint32_t  bfOffBits;
} __attribute__ ((__packed__)) bitmapFileHeader_t;

typedef struct {
    uint32_t biSize;  //specifies the number of bytes required by the struct
    int32_t  biWidth;  //specifies width in pixels
    int32_t  biHeight;  //species height in pixels
    uint16_t biPlanes; //specifies the number of color planes, must be 1
    uint16_t biBitCount; //specifies the number of bit per pixel
    uint32_t biCompression;//spcifies the type of compression
    uint32_t biSizeImage;  //size of image in bytes
    int32_t  biXPelsPerMeter;  //number of pixels per meter in x axis
    int32_t  biYPelsPerMeter;  //number of pixels per meter in y axis
    uint32_t biClrUsed;  //number of colors used by th ebitmap
    uint32_t biClrImportant;  //number of colors that are important
} __attribute__ ((__packed__)) bitmapInfoHeader_t;

FILE *loadBitmapFile( char *filename, bitmapFileHeader_t *bitmapFileHeader, bitmapInfoHeader_t *bitmapInfoHeader );
void copyBmpToFbRect( FILE *bmpF, int xBmp, int yBmp, int wBmp, int hBmp, int xFb, int yFb, uint8_t layerFb, uint8_t rFb, uint8_t gFb, uint8_t bFb );


#endif