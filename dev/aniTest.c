#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
// File header
// DGD<16:nAnimations>
// 0x0000001B: < 2:HEADER_SIZE>	//???
// 0x000001EF: < 3:"B130"> 		//build string
// startup animation
// 0x00001400: < 7:"RUN_DMD"> 	//marker of begin of retentive settings area?
// 0x0000C600:					//Unused OS bootsector data
// 0x0000C800:					//Begin of animation table
// 0x000E8000:					//Looks like a crapload of font data!
// 0x004E2000:					//First animation frame

// 0x00002710:					//First raw offset
// --> MAGIC_OFFSET = 0


#define HEADER_OFFS   0x0000C800
#define HEADER_SIZE   0x00000200
#define ANI_DAT_START 0x004E2000

#define SWAP16(x) ( (x>>8) | (x<<8) )
#define SWAP32(x) ( ((x>>24)&0xff) | ((x<<8)&0xff0000) | ((x>>8)&0xff00) | ((x<<24)&0xff000000) )

typedef struct{
	uint16_t nAnimations;
	char     buildStr[8];
} fileHeader_t;

// Type of the frame header table
typedef struct{
	uint8_t frameId;			// 1-based frame index
	uint8_t frameDur;			// Frame duration [ms]
} frameHeaderEntry_t;

typedef struct {
	uint16_t animationId;
	uint8_t  unknown0;			//almost always 1 (maybe `frameTablePresent`)
	uint8_t  nStoredFrames;
	uint32_t byteOffset;		//points to frameTable
	uint8_t  nFrameEntries;		//in the frame table
	uint8_t  width;				//[pixels]
	uint8_t  height;			//[pixels]
	uint8_t  unknown1[9];
	char     name[32];          //zero terminated
	frameHeaderEntry_t *frameHeader;// the frame table
} __attribute__ ((__packed__)) headerEntry_t;

int getFileHeader( FILE *f, fileHeader_t *fh ){
	char tempCh[3];
	fseek( f, 0x00000000, SEEK_SET );
	fread( tempCh, 3, 1, f );
	if( memcmp(tempCh,"DGD", 3) != 0 ){
		printf("Invalid file header!");
		return -1;
	}
	fread( &fh->nAnimations, 2, 1, f );
	fh->nAnimations = SWAP16( fh->nAnimations );
	fseek( f, 0x000001EF, SEEK_SET );
	fread( &fh->buildStr, 8, 1, f );
	return 0;
}

int readHeaderEntry( FILE *f, headerEntry_t *h, int headerIndex ){
	// Copys header info into h. Note that h->nFrameEntries must be freed!
	// leaves file f seeked to the beginning of the animation data
	fseek( f, HEADER_OFFS + HEADER_SIZE*headerIndex, SEEK_SET );
	fread( h, sizeof(*h), 1, f );
	h->name[31] = '\0';
	h->frameHeader = NULL;
	h->animationId = SWAP16(h->animationId);
	h->byteOffset =  SWAP32(h->byteOffset)*HEADER_SIZE;
	// Exract frame header
	frameHeaderEntry_t *fh = malloc( sizeof(frameHeaderEntry_t) * h->nFrameEntries );
	if ( fh == NULL ){
		printf("Memory allocation error!");
		return -1;
	}
	h->frameHeader = fh;
	fseek( f, h->byteOffset, SEEK_SET );
	for( int i=0; i<h->nFrameEntries; i++ ){
		fread( &fh->frameId,  1, 1, f );
		fread( &fh->frameDur, 1, 1, f );
		// Hack to sort out invalid headers
		if( fh->frameDur <= 0 || fh->frameId > h->nStoredFrames ){
			printf("Invalid header!");
			return -1;
		}		
		fh++;
	}
	return 0;
}

int main(int argc, char *argv[]){
	if( argc != 2 ){
		printf("usage: %s runDmdSdCard.img\n", argv[0]);
		return 0;
	}
	FILE *f = fopen( argv[1], "r" );
	fileHeader_t fh;
	getFileHeader( f, &fh );
	printf("Build: %s, Found %d animations\n\n", fh.buildStr, fh.nAnimations);
	fseek( f, HEADER_OFFS, SEEK_SET );
	headerEntry_t myHeader;
	for( int i=0; i<fh.nAnimations; i++ ){
		readHeaderEntry( f, &myHeader, i );
		printf("%s\n", myHeader.name);
		printf("--------------------------------\n");
		printf("animationId:       0x%04X\n", myHeader.animationId);
		printf("nStoredFrames:         %d\n", myHeader.nStoredFrames);
		printf("byteOffset:    0x%08X\n", myHeader.byteOffset);
		printf("nFrameEntries:         %d\n", myHeader.nFrameEntries);
		printf("width:               0x%02X\n", myHeader.width);
		printf("height:              0x%02X\n", myHeader.height);
		printf("unknowns: %02X ", myHeader.unknown0 );
		for( int j=0; j<sizeof(myHeader.unknown1); j++) printf("%02X ", myHeader.unknown1[j]);
		printf("\n");
		if( myHeader.byteOffset < ANI_DAT_START ) printf("!!! Frame data missing !!!\n");
		for( int j=0; j<myHeader.nFrameEntries; j++ ){
			frameHeaderEntry_t fh = myHeader.frameHeader[j];
			printf("%3u:%3u ", fh.frameId, fh.frameDur );
			if( (j+1)%8 == 0 )
				printf("\n");
		}
		free( myHeader.frameHeader );
		myHeader.frameHeader = NULL;
		printf("\n\n");
	}
	fclose(f);
	return 0;
}
