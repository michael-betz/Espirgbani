//  draws bitmap fonts

#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "web_console.h"
#include "esp_heap_caps.h"
#include "i2s_parallel.h"
#include "rgb_led_panel.h"
#include "frame_buffer.h"
#include "bmp.h"

static const char *T = "BMP";

FILE *g_bmpFile = NULL;
bitmapFileHeader_t g_bmpFileHeader;
bitmapInfoHeader_t g_bmpInfoHeader;
font_t         *g_fontInfo;

// frees up allocated memory of an font_t object
void freeFdat( font_t *fDat ){
    if( fDat != NULL ){
        if( fDat->info != NULL )
            free( fDat->info );
        if( fDat->common != NULL )
            free( fDat->common );
        if( fDat->pageNames != NULL )
            free( fDat->pageNames );
        if( fDat->chars != NULL )
            free( fDat->chars );
        if( fDat->kerns != NULL )
            free( fDat->kerns );
        free( fDat );
    }
}

// loads a binary angelcode .fnt file into a font_t object
font_t *loadFntFile( char *fileName ){
    font_t *fDat = NULL;
    uint8_t temp[4], blockType;
    uint32_t blockSize;
    FILE *f = fopen( fileName, "rb" );
    fread( temp, 4,1,f);
    if( strncmp((char*)temp, "BMF", 3) != 0 ){
        printf("Wrong header\n");
        fclose( f );
        return NULL;
    }
    fDat = malloc( sizeof(font_t) );
    memset( fDat, 0x00, sizeof(font_t) );
    while(1){
        blockType = 0;
        if( fread( &blockType, 1,1,f) < 1 ){
            printf("No more data :(\n");
            break;
        }
        fread( &blockSize, 1, 4, f );
        printf("block %d of size %d\n", blockType, blockSize);
        switch( blockType ){
            case 1:
                fDat->info = malloc( blockSize );
                fread( fDat->info, 1, blockSize, f );
                break;

            case 2:
                fDat->common = malloc( blockSize );
                fread( fDat->common, 1, blockSize, f );
                break;

            case 3:
                fDat->pageNames = malloc( blockSize );
                fDat->pageNamesLen = blockSize;
                fread( fDat->pageNames, 1, blockSize, f );
                break;

            case 4:
                fDat->chars = malloc( blockSize );
                fDat->charsLen = blockSize;
                fread( fDat->chars, 1, blockSize, f );
                break;

            case 5:
                fDat->kerns = malloc( blockSize );
                fDat->kernsLen = blockSize;
                fread( fDat->kerns, 1, blockSize, f );
                break;

            default:
                printf("Unknown block type: %d\n", blockType );
                fseek( f, blockSize, SEEK_CUR );
                continue;
        }
    }
    fclose( f );
    return fDat;
}

// prints (some) content of a font_t object
void printFontFile( font_t *fDat ){
    ESP_LOGI(T, "--------------------------------------");
    ESP_LOGI(T, "fontSize: %d", fDat->info->fontSize );
    ESP_LOGI(T, "aa: %d", fDat->info->aa );
    ESP_LOGI(T, "name: %s", fDat->info->fontName );
    ESP_LOGI(T, "--------------------------------------");
    ESP_LOGI(T, "lineHeight: %d", fDat->common->lineHeight );
    ESP_LOGI(T, "scaleW: %d", fDat->common->scaleW );
    ESP_LOGI(T, "scaleH: %d", fDat->common->scaleH );
    ESP_LOGI(T, "pages: %d", fDat->common->pages );
    ESP_LOGI(T, "--------------------------------------");
    ESP_LOGI(T, "pagenames[0]: %s", fDat->pageNames );
    // for( int i=0; i<fDat->pageNamesLen; i++ ){
    //     ESP_LOGI(T, "%c", fDat->pageNames[i] );
    // }
    ESP_LOGI(T, "--------------------------------------");
    int nChars = fDat->charsLen/(int)sizeof(fontChar_t);
    ESP_LOGI(T, "chars: %d", nChars );
    for( int i=0; i<nChars; i++ )
        ESP_LOGI(T, "id: %3d, x: %3d, y: %3d, w: %3d, h: %3d", fDat->chars[i].id, fDat->chars[i].x, fDat->chars[i].y, fDat->chars[i].width, fDat->chars[i].height );
    ESP_LOGI(T, "--------------------------------------");
}

// get pointer to fontChar_t for a specific character, or NULL if not found
fontChar_t *getCharInfo( font_t *fDat, char c ){
    fontChar_t *tempC = fDat->chars;
    int nChars = fDat->charsLen/(int)sizeof(fontChar_t);
    for( int i=0; i<nChars; i++ ){
        if( tempC->id == c ){
            return tempC;
        }
        tempC++;
    }
    return NULL;
}

// Loads a <prefix>.bmp and <prefix>.fnt file
void initFont( char *filePrefix ){
    char tempFileName[32];
    if ( g_bmpFile != NULL ){
        fclose( g_bmpFile );
        g_bmpFile = NULL;
    }
    sprintf( tempFileName, "/SD/%s.fnt", filePrefix );
    g_fontInfo = loadFntFile( tempFileName );
    if( g_fontInfo == NULL ){
        ESP_LOGE(T, "Could not load %s", tempFileName);
        return;
    }
    sprintf( tempFileName, "/SD/%s.bmp", filePrefix );
    g_bmpFile = loadBitmapFile( tempFileName, &g_bmpFileHeader, &g_bmpInfoHeader );
    if( g_bmpFile == NULL ){
        ESP_LOGE(T, "Could not load %s", tempFileName);
        return;
    }
}

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
void copyBmpToFbRect( FILE *bmpF, bitmapInfoHeader_t *bmInfo, int xBmp, int yBmp, int wBmp, int hBmp, int xFb, int yFb, uint8_t layerFb, uint8_t rFb, uint8_t gFb, uint8_t bFb ){
    if ( bmpF==NULL || bmInfo==NULL )
        return;
    int rowSize = ( (bmInfo->biBitCount * bmInfo->biWidth + 31)/32 * 4 );
    uint8_t *rowBuffer = malloc( rowSize );
    if( rowBuffer==NULL )
        return;
	int startPos = ftell( bmpF );
	//move vertically to the right row
    fseek( bmpF, rowSize*(bmInfo->biHeight-yBmp-hBmp), SEEK_CUR );
    for ( int rowId=0; rowId<hBmp; rowId++ ){
    	//read the whole row
    	fread( rowBuffer, rowSize, 1, bmpF );
    	//copy the relevant pixels
        uint8_t *p = &rowBuffer[ xBmp*(bmInfo->biBitCount/8) ];
		for ( int colId=0; colId<wBmp; colId++ ){
			setPixel( layerFb, colId+xFb, hBmp-rowId-1+yFb, rFb, gFb, bFb, 255-(*p) );
            p += bmInfo->biBitCount/8;
		}
    }
    fseek( bmpF, startPos, SEEK_SET );
    free( rowBuffer );
}