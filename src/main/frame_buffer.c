// Implements a framebuffer with N layers and alpha blending

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

// static const char *T = "FRAME_BUFFER";
EventGroupHandle_t layersDoneDrawingFlags = NULL;

uint32_t g_frameBuff[N_LAYERS][DISPLAY_WIDTH*DISPLAY_HEIGHT]; // framebuffer with `N_LAYERS` in MSB ABGR LSB format

void initFb(){
	layersDoneDrawingFlags = xEventGroupCreate();
	for( int i=0; i<N_LAYERS; i++ )
		xEventGroupSetBits( layersDoneDrawingFlags, (1<<i) );
}

void startDrawing( uint8_t layer ){
	xEventGroupWaitBits( layersDoneDrawingFlags, (1<<layer), pdTRUE, pdTRUE, 3000/portTICK_PERIOD_MS );
}

void doneDrawing( uint8_t layer ){
	xEventGroupSetBits( layersDoneDrawingFlags, (1<<layer) );
}

void waitDrawingDone(){
	uint8_t bitFlags = 0;
	for( int i=0; i<N_LAYERS; i++ )
		bitFlags = (bitFlags<<1) | 1;
	// All layer bits must be set (not busy)
	xEventGroupWaitBits( layersDoneDrawingFlags, bitFlags, pdTRUE, pdTRUE, 3000/portTICK_PERIOD_MS ); // wait for ongoing drawings
	// All layer bits are cleared now (busy)
}

void doneUpdating(){
	uint8_t bitFlags = 0;
	for( int i=0; i<N_LAYERS; i++ )
		bitFlags = (bitFlags<<1) | 1;
	// Set all layer bits again
	xEventGroupSetBits( layersDoneDrawingFlags, bitFlags );	// New drawings ok
}

#define BLEND_LAYERS(t,m,b) ( (t*(255-m)+b*m)/255 )
#define GR(p) ( p     &0xFF)
#define GG(p) ((p>> 8)&0xFF)
#define GB(p) ((p>>16)&0xFF)
#define GA(p) ((p>>24)&0xFF)

//Get a blended pixel from the 2 layers of frameBuffer, assuming the image is a DISPLAY_WIDTHx32 8R8G8B image
//Returns it as an uint32 with the lower 24 bits containing the RGB values.
uint32_t getBlendedPixel( int x, int y ) {
    uint8_t resR=0, resG=0, resB=0;
    for( uint8_t l=0; l<N_LAYERS; l++ ){
	    uint32_t p = g_frameBuff[l][x+y*DISPLAY_WIDTH];	// Get a pixel value of one layer
		resR = BLEND_LAYERS( GR(p), GA(p), resR );
		resG = BLEND_LAYERS( GG(p), GA(p), resG );
		resB = BLEND_LAYERS( GB(p), GA(p), resB );
    }
	return (resB<<16) | (resG<<8) | resR;
}

// Set a pixel in frmaebuffer at p
void setPixel( uint8_t layer, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a ) {
	if( layer >= N_LAYERS || x>=DISPLAY_WIDTH || y>=DISPLAY_HEIGHT ){
		return;
	}
    g_frameBuff[layer][x+y*DISPLAY_WIDTH] = (a<<24) | (b<<16) | (g<<8) | r;
}

// set all pixels of a layer to a color
void setAll( uint8_t layer, uint8_t r, uint8_t g, uint8_t b, uint8_t a ){
    if( layer >= N_LAYERS ){
		return;
	}
    uint32_t *p = (uint32_t*)g_frameBuff[layer];
    for( int i=0; i<DISPLAY_WIDTH*DISPLAY_HEIGHT; i++ ){
        *p++ = (a<<24) | (b<<16) | (g<<8) | r;
    }
}

uint32_t incrAlpha( uint8_t layer, uint8_t speed ){
    if( layer >= N_LAYERS ){
		return 0;
	}
	if( speed <= 0 )	speed = 1;
    uint32_t *p = (uint32_t*)g_frameBuff[layer];
    uint32_t nTouched=0;
    for( int i=0; i<DISPLAY_WIDTH*DISPLAY_HEIGHT; i++ ){
        int a = GA(*p);
        if( a < 255 ){
        	a += speed;
        	if( a > 255 )	a = 255;
        	*p &= 0x00FFFFFF;
        	*p |= a<<24;
        	nTouched++;
        }
        p++;
    }
    return nTouched;
}

// write scaled color values to framebuffer [RGBA]
#define SET_ANI_PIXEL( p, mr, mg, mb, pix ) { *p++ = (((pix==0x0A)?0xFF:0x00)<<24) | ((mb*pix)<<16) | ((mg*pix)<<8) | (mr*pix); }

void setFromFile( FILE *f, uint8_t layer, uint8_t r, uint8_t g, uint8_t b ){
    uint32_t *p = g_frameBuff[layer];
    uint8_t pix, pixTemp, mr=r/15, mg=g/15, mb=b/15;
    for( int y=0; y<DISPLAY_HEIGHT; y++ ){
        for( int x=0; x<DISPLAY_WIDTH; x+=2 ){
            fread( &pix, 1, 1, f );
            // unpack the 2 pixels per byte data into 1 pixel per byte and set the framebuffer
            pixTemp = pix >> 4;
            SET_ANI_PIXEL( p, mr, mg, mb, pixTemp );
            pixTemp = pix & 0x0F;
            SET_ANI_PIXEL( p, mr, mg, mb, pixTemp );
        }
    }
}
