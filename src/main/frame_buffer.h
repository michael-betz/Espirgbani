#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H
#include "rgb_led_panel.h"

#define N_LAYERS	3

extern uint32_t g_frameBuff[N_LAYERS][DISPLAY_WIDTH*DISPLAY_HEIGHT];

extern uint32_t getBlendedPixel( int x, int y );

// Set a single pixel on a layer to a specific RGBA color in the framebuffer
void setPixel( uint8_t layer, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a );

// Set whole layer to fixed color
extern void setAll( uint8_t layer, uint8_t r, uint8_t g, uint8_t b, uint8_t a );

// write image from a runDmd image file into layer with shades of color r g b
extern void setFromFile( FILE *f, uint8_t layer, uint8_t r, uint8_t g, uint8_t b );

// Increments the alpha value of every pixel by `speed` until it reaches 255
// returns the number of pixels which have been touched
uint32_t incrAlpha( uint8_t layer, uint8_t speed );


void initFb();
void startDrawing( uint8_t layer );
void doneDrawing( uint8_t layer );
void waitDrawingDone();
void doneUpdating();


#endif