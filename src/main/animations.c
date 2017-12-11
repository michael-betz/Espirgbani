#include <string.h>
#include "esp_log.h"
#include "animations.h"

static const char *T = "ANIMATIONS";

int getFileHeader( FILE *f, fileHeader_t *fh ){
	char tempCh[3];
	fseek( f, 0x00000000, SEEK_SET );
	fread( tempCh, 3, 1, f );
	if( memcmp(tempCh,"DGD", 3) != 0 ){
		ESP_LOGE( T, "Invalid file header!");
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
		ESP_LOGE( T, "Memory allocation error!");
		return -1;
	}
	h->frameHeader = fh;
	fseek( f, h->byteOffset, SEEK_SET );
	for( int i=0; i<h->nFrameEntries; i++ ){
		fread( &fh->frameId,  1, 1, f );
		fread( &fh->frameDur, 1, 1, f );
		// Hack to sort out invalid headers
		if( fh->frameDur <= 0 || fh->frameId > h->nStoredFrames ){
			ESP_LOGE( T, "Invalid header!");
			return -1;
		}		
		fh++;
	}
	fseek( f, h->byteOffset+HEADER_SIZE, SEEK_SET );
	return 0;
}
