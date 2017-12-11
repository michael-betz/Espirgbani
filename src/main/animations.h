#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include <stdio.h>

#define HEADER_OFFS   0x0000C800
#define HEADER_SIZE   0x00000200

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
	uint8_t  unknown0;			//almost always 1
	uint8_t  nStoredFrames;
	uint32_t byteOffset;		//points to frameTable
	uint8_t  nFrameEntries;		//in the frame table
	uint8_t  width;				//[pixels]
	uint8_t  height;			//[pixels]
	uint8_t  unknown1[9];
	char     name[32];          //zero terminated
	frameHeaderEntry_t *frameHeader;// the frame table
} __attribute__ ((__packed__)) headerEntry_t;

// Reads the filehader and fills `fh`
extern int getFileHeader( FILE *f, fileHeader_t *fh );

// Fills the headerEntry struct with data.
// Specify an `headerIndex` from 0 to nAnimations
extern int readHeaderEntry( FILE *f, headerEntry_t *h, int headerIndex );

#endif