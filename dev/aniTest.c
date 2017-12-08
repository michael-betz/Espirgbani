#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define HEADER_OFFS 0xC800
#define HEADER_SIZE 0x0200

#define SWAP16(x) ( (x>>8) | (x<<8) )
#define SWAP32(x) ( ((x>>24)&0xff) | ((x<<8)&0xff0000) | ((x>>8)&0xff00) | ((x<<24)&0xff000000) )

typedef struct {
	uint16_t animationId;
	uint8_t  nStoredFrames;
	uint32_t byteOffset;		//points to frameTable
	uint8_t  nFrameEntries;		//in the frame table
	uint8_t  width;				//[pixels]
	uint8_t  height;			//[pixels]
	char     name[32];          //zero terminated
} headerEntry_t;

int countHeaderEntries( FILE *f ){
	uint32_t aniIdExpected = 1;
	uint16_t aniId=0;
	fseek( f, HEADER_OFFS, SEEK_SET );
	while( aniId < 0xFFFF ){
		fread( &aniId, sizeof(aniId), 1, f );
		aniId = SWAP16(aniId);
		if( aniId != aniIdExpected ){
			return aniIdExpected-1;
		}
		aniIdExpected++;
		fseek( f, 0x200-2, SEEK_CUR );
	}
}

int readHeaderEntry( FILE *f, headerEntry_t *h, int headerIndex ){
	uint32_t temp=0;
	memset( h, 0, sizeof(*h) );
	long startPos = HEADER_OFFS + HEADER_SIZE*headerIndex;
	fseek( f, startPos, SEEK_SET );
	uint16_t aniId;
	fread( &aniId,             sizeof(h->animationId),   1, f );
	aniId = SWAP16(aniId);
	h->animationId = aniId;
	fread( &temp, 1, 1, f ); // Almost always 1
	fread( &h->nStoredFrames, sizeof(h->nStoredFrames), 1, f );
	fread( &temp,             sizeof(h->byteOffset),    1, f );
	h->byteOffset = SWAP32(temp)*0x200 - 0xB58000;
	fread( &h->nFrameEntries, sizeof(h->nFrameEntries), 1, f );
	fread( &h->width,         sizeof(h->width),         1, f );
	fread( &h->height,        sizeof(h->height),        1, f );
	fseek( f, startPos+0x14, SEEK_SET );
	fread(  h->name,          sizeof(h->name),          1, f );
	return 0;
}

int main(int argc, char *argv[]){
	if( argc != 2 ){
		printf("usage: %s runDmdSdCard.img\n", argv[0]);
		return 0;
	}
	printf("Hello World!\n");
	FILE *f = fopen( argv[1], "r" );
	int hCnt = countHeaderEntries( f );
	printf("Found %d headers\n", hCnt);
	fseek( f, HEADER_OFFS, SEEK_SET );
	headerEntry_t myHeader;
	for( int i=0; i<hCnt; i++ ){
		readHeaderEntry( f, &myHeader, i );
		printf("animationId:   0x%04X\n", myHeader.animationId);
		printf("nStoredFrames: %d\n", myHeader.nStoredFrames);
		printf("byteOffset:    0x%08X\n", myHeader.byteOffset);
		printf("nFrameEntries: %d\n", myHeader.nFrameEntries);
		printf("width:         %d\n", myHeader.width);
		printf("height:        %d\n", myHeader.height);
		printf("name:          %s\n", myHeader.name);
		printf("\n");
	}
	fclose(f);
	return 0;
}
