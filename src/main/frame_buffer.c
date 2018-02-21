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

// framebuffer with `N_LAYERS` in MSB ABGR LSB format
// Colors are premultiplied with their alpha values for easiser compositing
uint32_t g_frameBuff[N_LAYERS][DISPLAY_WIDTH*DISPLAY_HEIGHT]; 

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

//Get a blended pixel from the N layers of frameBuffer, 
// assuming the image is a DISPLAY_WIDTHx32 8A8R8G8B image. Color values are premultipleid with alpha
// Returns it as an uint32 with the lower 24 bits containing the RGB values.
uint32_t getBlendedPixel( int x, int y ) {
    uint8_t resR=0, resG=0, resB=0;
    for( uint8_t l=0; l<N_LAYERS; l++ ){
        uint32_t p = g_frameBuff[l][x+y*DISPLAY_WIDTH]; // Get a pixel value of one layer
        resR = INT_PRELERP( resR, GR(p), GA(p) );
        resG = INT_PRELERP( resG, GG(p), GA(p) );
        resB = INT_PRELERP( resB, GB(p), GA(p) );
    }
    return (resB<<16) | (resG<<8) | resR;
}

// Set a pixel in frmaebuffer at p
void setPixel( uint8_t layer, uint16_t x, uint16_t y, uint32_t color ) {
	if( x>=DISPLAY_WIDTH || y>=DISPLAY_HEIGHT || layer>=N_LAYERS ){ return;	}
    g_frameBuff[layer][x+y*DISPLAY_WIDTH] = color; //(a<<24) | (b<<16) | (g<<8) | r;
}

void setPixelOver( uint8_t layer, uint16_t x, uint16_t y, uint32_t color ) {
    if( x>=DISPLAY_WIDTH || y>=DISPLAY_HEIGHT || layer>=N_LAYERS ){ return; }
    uint32_t p = g_frameBuff[layer][x+y*DISPLAY_WIDTH];
    uint8_t resR = INT_PRELERP( GR(p), GR(color), GA(color) );
    uint8_t resG = INT_PRELERP( GG(p), GG(color), GA(color) );
    uint8_t resB = INT_PRELERP( GB(p), GB(color), GA(color) );
    uint8_t resA = INT_PRELERP( GA(p), GA(color), GA(color) );
    g_frameBuff[layer][x+y*DISPLAY_WIDTH] = SRGBA( resR, resG, resB, resA );
}

// set all pixels of a layer to a color
void setAll( uint8_t layer, uint32_t color ){
    if( layer >= N_LAYERS ){
		return;
	}
    uint32_t *p = (uint32_t*)g_frameBuff[layer];
    for( int i=0; i<DISPLAY_WIDTH*DISPLAY_HEIGHT; i++ ){
        *p++ = color;
    }
}

uint32_t fadeOut( uint8_t layer, uint8_t factor ){
    if( layer >= N_LAYERS ){
		return 0;
	}
	if( factor <= 0 )	factor = 1;
    uint8_t scale = 255-factor;
    uint32_t *p = (uint32_t*)g_frameBuff[layer];
    uint32_t nTouched=0;
    for( int i=0; i<DISPLAY_WIDTH*DISPLAY_HEIGHT; i++ ){
        if( *p > 0 ){
            *p = scale32( scale, *p );
            nTouched++;
        }
        p++;
    }
    return nTouched;
}

// take a shade (0-15) from animation file and 24 bit RGB color. Return a RGBA color.
#define GET_ANI_COLOR( p, c ) ( ((p)==0x0A)?(0):(scale32(p<<4,c)|0xFF000000) )

void setFromFile( FILE *f, uint8_t layer, uint32_t color ){
    uint32_t *p = g_frameBuff[layer];
    uint8_t pix, pixTemp;
    for( int y=0; y<DISPLAY_HEIGHT; y++ ){
        for( int x=0; x<DISPLAY_WIDTH; x+=2 ){
            fread( &pix, 1, 1, f );
            // unpack the 2 pixels per byte data into 1 pixel per byte and set the framebuffer
            pixTemp = pix >> 4;
            *p++ = GET_ANI_COLOR( pixTemp, color );
            pixTemp = pix & 0x0F;
            *p++ = GET_ANI_COLOR( pixTemp, color );
        }
    }
}
