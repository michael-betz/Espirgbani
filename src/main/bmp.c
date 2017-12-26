//  opens bitmap (.bmp) files

#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "frame_buffer.h"
#include "bmp.h"

static const char *T = "BMP";

// Returns a filepointer seeked to the beginngin of the bitmap data
FILE *loadBitmapFile( char *filename, bitmapFileHeader_t *bitmapFileHeader, bitmapInfoHeader_t *bitmapInfoHeader ) {
    FILE *filePtr; //our file pointer
    //open filename in read binary mode
    filePtr = fopen(filename,"rb");
    if (filePtr == NULL)
        return NULL;
    //read the bitmap file header
    fread( bitmapFileHeader, sizeof(bitmapFileHeader_t),1,filePtr);
    //verify that this is a bmp file by check bitmap id
    if (bitmapFileHeader->bfType !=0x4D42) {
        fclose(filePtr);
        return NULL;
    }
    //read the bitmap info header
    fread(bitmapInfoHeader, sizeof(bitmapInfoHeader_t),1,filePtr);
    //move file point to the beggining of bitmap data
    fseek(filePtr, bitmapFileHeader->bfOffBits, SEEK_SET);
    return filePtr;
}

// copys a rectangular area from a bitmap to the frambuffer layer
void copyBmpToFbRect( FILE *bmpF, bitmapInfoHeader_t *bmInfo, uint16_t xBmp, uint16_t yBmp, uint16_t w, uint16_t h, uint16_t xFb, uint16_t yFb, uint8_t layerFb, uint8_t rFb, uint8_t gFb, uint8_t bFb ){
    if ( bmpF==NULL || bmInfo==NULL )
        return;
    int rowSize = ( (bmInfo->biBitCount * bmInfo->biWidth + 31)/32 * 4 );
    uint8_t *rowBuffer = malloc( rowSize );
    if( rowBuffer==NULL ){
        ESP_LOGE(T, "Could not allocate rowbuffer");
        return;
    }
	int startPos = ftell( bmpF );
	//move vertically to the right row
    fseek( bmpF, rowSize*(bmInfo->biHeight-yBmp-h), SEEK_CUR );
    for ( int rowId=0; rowId<h; rowId++ ){
    	//read the whole row
    	if( fread( rowBuffer, 1, rowSize, bmpF ) != rowSize ){
            fseek( bmpF, startPos, SEEK_SET );               // skipped over last row
            free( rowBuffer );        
            return; 
        }
    	//copy one row of relevant pixels
		for ( uint16_t colId=0; colId<w; colId++ ){
            uint16_t rowAddr = (xBmp+colId) * (bmInfo->biBitCount/8);
            if( rowAddr >= rowSize ){
                break;
            }
			setPixel( layerFb, colId+xFb, h-rowId-1+yFb, rFb, gFb, bFb, 255-(rowBuffer[rowAddr]) );
		}
    }
    fseek( bmpF, startPos, SEEK_SET );
    free( rowBuffer );
}