#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
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
        ESP_LOGE(T,"Wrong header");
        fclose( f );
        return NULL;
    }
    fDat = malloc( sizeof(font_t) );
    memset( fDat, 0x00, sizeof(font_t) );
    while(1){
        blockType = 0;
        if( fread( &blockType, 1,1,f) < 1 ){
            ESP_LOGV(T,"No more data");
            break;
        }
        fread( &blockSize, 1, 4, f );
        ESP_LOGD(T,"block %d of size %d", blockType, blockSize);
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
                ESP_LOGW(T,"Unknown block type: %d\n", blockType );
                fseek( f, blockSize, SEEK_CUR );
                continue;
        }
    }
    fclose( f );
    return fDat;
}

// prints (some) content of a font_t object
void printFntFile( font_t *fDat ){
    ESP_LOGD(T, "--------------------------------------");
    ESP_LOGD(T, "fontSize: %d", fDat->info->fontSize );
    ESP_LOGD(T, "aa: %d", fDat->info->aa );
    ESP_LOGD(T, "name: %s", fDat->info->fontName );
    ESP_LOGD(T, "--------------------------------------");
    ESP_LOGD(T, "lineHeight: %d", fDat->common->lineHeight );
    ESP_LOGD(T, "scaleW: %d", fDat->common->scaleW );
    ESP_LOGD(T, "scaleH: %d", fDat->common->scaleH );
    ESP_LOGD(T, "pages: %d", fDat->common->pages );
    ESP_LOGD(T, "--------------------------------------");
    ESP_LOGD(T, "pagenames[0]: %s", fDat->pageNames );
    // for( int i=0; i<fDat->pageNamesLen; i++ ){
    //     ESP_LOGD(T, "%c", fDat->pageNames[i] );
    // }
    ESP_LOGD(T, "--------------------------------------");
    int nChars = fDat->charsLen/(int)sizeof(fontChar_t);
    ESP_LOGV(T, "chars: %d", nChars );
    for( int i=0; i<nChars; i++ )
        ESP_LOGV(T, "id: %3d, x: %3d, y: %3d, w: %3d, h: %3d", fDat->chars[i].id, fDat->chars[i].x, fDat->chars[i].y, fDat->chars[i].width, fDat->chars[i].height );
    ESP_LOGV(T, "--------------------------------------");
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
void initFont( const char *filePrefix ){
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
    printFntFile( g_fontInfo );
    sprintf( tempFileName, "%s_0.bmp", filePrefix );
    ESP_LOGI(T, "Loading %s", tempFileName);
    g_bmpFile = loadBitmapFile( tempFileName, &g_bmpFileHeader, &g_bmpInfoHeader );
    if( g_bmpFile == NULL ){
        ESP_LOGE(T, "Could not load %s", tempFileName);
        return;
    }
}

int cursX=0, cursY=8;
char g_lastChar = 0;

void setCur( int x, int y ){
	cursX = x;
	cursY = y;
	g_lastChar = 0;
}

void drawChar( char c, uint8_t layer, uint32_t color, uint8_t chOffset ){
	if( g_bmpFile==NULL || g_fontInfo==NULL ) return;
	if( c == '\n' ){
		cursY += g_fontInfo->common->lineHeight;
	} else {
		fontChar_t *charInfo = getCharInfo( g_fontInfo, c );
		// int16_t k = getKerning( g_lastChar, c );
		if( charInfo ){
			copyBmpToFbRect( g_bmpFile,
							 &g_bmpInfoHeader,
							 charInfo->x,
							 charInfo->y,
							 charInfo->width,
							 charInfo->height,
							 cursX + charInfo->xoffset,
							 cursY + charInfo->yoffset,
							 layer, color, chOffset );
			cursX += charInfo->xadvance;
		}
	}
	g_lastChar = c;
}

int getStrWidth( const char *str ){
    int w=0;
    while( *str ){
        fontChar_t *charInfo = getCharInfo( g_fontInfo, *str );
        if( charInfo )
            w += charInfo->xadvance;
        str++;
    }
    return w;
}

void drawStr( const char *str, int x, int y, uint8_t layer, uint32_t cOutline, uint32_t cFill ){
	const char *c = str;
    ESP_LOGI(T, "(%d,%d): %s", x, y, str );
    setCur( x, y );
	while( *c ){
        // Render outline first (from green channel)
        // Note that .bmp color order is  Blue, Green, Red
		drawChar( *c++, layer, cOutline, 1 );
	}
    c = str;
    setCur( x, y );
    while( *c ){
        // Render filling (from red channel)
        drawChar( *c++, layer, cFill, 2 );
    }
}

void drawStrCentered( const char *str, uint8_t layer, uint32_t cOutline, uint32_t cFill ){
    int w, h, xOff, yOff;
    h = g_fontInfo->common->lineHeight;
    w = getStrWidth( str );
    ESP_LOGI(T, "getStrDim( w = %d, h = %d )", w, h );
    xOff = (DISPLAY_WIDTH-w)/2;
    yOff = (DISPLAY_HEIGHT-h)/2;
    startDrawing( layer );
    setAll( layer, 0x00000000 );
    drawStr( str, xOff, yOff, layer, cOutline, cFill );
    doneDrawing( layer );
}

// Returns the number of consecutive `path/0.fnt` files
int cntFntFiles( const char* path ){
    int nFiles = 0;
    char fNameBuffer[32];
    struct stat   buffer;
    while( 1 ){
        sprintf( fNameBuffer, "%s/%d.fnt", path, nFiles );
        if( stat(fNameBuffer, &buffer) == 0 ) {
            nFiles++;
        } else {
            break;
        }
    }
    return nFiles;
}
