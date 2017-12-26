#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "frame_buffer.h"
#include "bmp.h"
#include "font.h"

static const char *T = "FONT";

FILE 				*g_bmpFile = NULL;
bitmapFileHeader_t 	g_bmpFileHeader;
bitmapInfoHeader_t 	g_bmpInfoHeader;
font_t         		*g_fontInfo = NULL;

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
void printFntFile( font_t *fDat ){
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

// frees up allocated memory of an font_t object
void freeFntFile( font_t *fDat ){
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

// Loads a <prefix>.bmp and <prefix>.fnt file, to get ready for printing
void initFont( char *filePrefix ){
    char tempFileName[32];
    if ( g_bmpFile != NULL ){
        fclose( g_bmpFile );
        g_bmpFile = NULL;
    }
    sprintf( tempFileName, "%s.fnt", filePrefix );
    ESP_LOGI(T, "Loading %s", tempFileName);
    g_fontInfo = loadFntFile( tempFileName );
    if( g_fontInfo == NULL ){
        ESP_LOGE(T, "Could not load %s", tempFileName);
        return;
    }
    sprintf( tempFileName, "%s.bmp", filePrefix );
    ESP_LOGI(T, "Loading %s", tempFileName);
    g_bmpFile = loadBitmapFile( tempFileName, &g_bmpFileHeader, &g_bmpInfoHeader );
    if( g_bmpFile == NULL ){
        ESP_LOGE(T, "Could not load %s", tempFileName);
        return;
    }
}

uint16_t cursX=0, cursY=8;
char g_lastChar = 0;

void setCur( uint16_t x, uint16_t y ){
	cursX = x;
	cursY = y;
	g_lastChar = 0;
}

void drawChar( char c, uint8_t layer, uint8_t r, uint8_t g, uint8_t b ){
	if( g_bmpFile==NULL || g_fontInfo==NULL ) return;
	if( c == '\n' ){
		cursY += g_fontInfo->common->lineHeight;
	} else {
		fontChar_t *charInfo = getCharInfo( g_fontInfo, c );
		// int16_t k = getKerning( g_lastChar, c );
		if( charInfo != NULL ){
			copyBmpToFbRect( g_bmpFile,
							 &g_bmpInfoHeader,
							 charInfo->x, 
							 charInfo->y,
							 charInfo->width,
							 charInfo->height,
							 cursX + charInfo->xoffset,
							 cursY + charInfo->yoffset,
							 layer, r, g, b );
			cursX += charInfo->xadvance;
		}
	}
	g_lastChar = c;
}

void drawStr( const char *str, uint16_t x, uint16_t y, uint8_t layer, uint8_t r, uint8_t g, uint8_t b ){
	ESP_LOGI(T, "(%d,%d): %s", x, y, str );
    setCur( x, y );
	while( *str ){
		drawChar( *str++, layer, r, g, b );
	}
}

uint16_t getStrWidth( const char *str ){
    uint16_t w = 0;
    while( *str ){
        fontChar_t *charInfo = getCharInfo( g_fontInfo, *str );
        w += charInfo->xadvance;
        str++;
    }
    return( w );
}