#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "bmp.h"

#define SWAP16(x) ( (x>>8) | (x<<8) )
#define SWAP32(x) ( ((x>>24)&0xff) | ((x<<8)&0xff0000) | ((x>>8)&0xff00) | ((x<<24)&0xff000000) )

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

void hexdump( uint8_t *dat, int len ){
	for( int i=0; i<len; i++){
		printf( " %02X", *dat++ );
		if( (i+1)%16==0 ){
			printf("\n");
		}
	}
	printf("\n");
}

font_t *loadFntFile( char *fileName ){
	font_t *fDat = NULL;
	uint8_t temp[64], blockType;
	int ret;
	uint32_t blockSize;
    FILE *f = fopen( fileName, "rb" );
    fread( temp, 4,1,f);
    if( strncmp(temp, "BMF", 3) != 0 ){
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
				ret = fread( fDat->info, 1, blockSize, f );
				break;

			case 2:
				fDat->common = malloc( blockSize );
				ret = fread( fDat->common, 1, blockSize, f );
				break;

			case 3:
				fDat->pageNames = malloc( blockSize );
				fDat->pageNamesLen = blockSize;
				ret = fread( fDat->pageNames, 1, blockSize, f );
				break;

			case 4:
				fDat->chars = malloc( blockSize );
				fDat->charsLen = blockSize;
				ret = fread( fDat->chars, 1, blockSize, f );
				break;

			case 5:
				fDat->kerns = malloc( blockSize );
				fDat->kernsLen = blockSize;
				ret = fread( fDat->kerns, 1, blockSize, f );
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

void printFontFile( font_t *fDat ){
	printf("--------------------------------------\n");
	printf("fontSize: %d\n", fDat->info->fontSize );
	printf("aa: %d\n", fDat->info->aa );
	printf("name: %s\n", fDat->info->fontName );
	printf("--------------------------------------\n");
	printf("lineHeight: %d\n", fDat->common->lineHeight );
	printf("scaleW: %d\n", fDat->common->scaleW );
	printf("scaleH: %d\n", fDat->common->scaleH );
	printf("pages: %d\n", fDat->common->pages );
	printf("--------------------------------------\n");
	printf("pagenames: ");
	for( int i=0; i<fDat->pageNamesLen; i++ ){
		printf("%c", fDat->pageNames[i] );
	}
	printf("\n");
	printf("--------------------------------------\n");
	int nChars = fDat->charsLen/(int)sizeof(fontChar_t);
	printf("chars: %d\n", nChars );
	for( int i=0; i<nChars; i++ )
		printf("id: %3d, x: %3d, y: %3d, w: %3d, h: %3d\n", fDat->chars[i].id, fDat->chars[i].x, fDat->chars[i].y, fDat->chars[i].width, fDat->chars[i].height );
	printf("--------------------------------------\n");
}

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

int main(int argc, char *argv[]){
	if( argc != 2 ){
		printf("usage: %s bla.fnt\n", argv[0]);
		return 0;
	}
	font_t *fDat = loadFntFile( argv[1] );
	printFontFile( fDat );
	char c = '8';
	printf("%c:\n", c);
	fontChar_t *tempChar = getCharInfo( fDat, c );
	if( tempChar != NULL )
		printf("id: %3d, x: %3d, y: %3d, w: %3d, h: %3d\n", tempChar->id, tempChar->x, tempChar->y, tempChar->width, tempChar->height );
	freeFdat( fDat );
	fDat = NULL;
	return 0;
}
