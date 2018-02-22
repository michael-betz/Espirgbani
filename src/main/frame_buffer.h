#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H
#include "rgb_led_panel.h"

#define N_LAYERS	3

#define BLEND_LAYERS(t,m,b) ( (t*(255-m)+b*m)/255 )

// Fast integer `scaling` a*b (255*255=255)
// from http://stereopsis.com/doubleblend.html
#define INT_MULT(a,b,t)        ( ((a)+1)*(b) >> 8 )
#define INT_PRELERP(p, q, a)   ( (p) + (q) - INT_MULT(a, p, 0) )

// from http://www.cs.princeton.edu/courses/archive/fall00/cs426/papers/smith95a.pdf
// t = 16 bit temporary variable.
// #define INT_MULT(a,b,t) ( (t)=(a)*(b)+0x80, ( (((t)>>8)+(t))>>8 ) )
// #define INT_PRELERP(p, q, a, t) ( (p) + (q) - INT_MULT( a, p, t) )
// B over A (premultipied alpha):  C' = INT_PRELERP( A', B', beta, t )

//Scales all 4 components in p with the same scaling factor scale (255*255=255)
static inline uint32_t scale32(uint32_t scale, uint32_t p) {
    scale += 1;
    uint32_t ag = (p>>8) & 0x00FF00FF;
    uint32_t rb =  p     & 0x00FF00FF;
    uint32_t sag = scale * ag;
    uint32_t srb = scale * rb;
    sag =  sag     & 0xFF00FF00;
    srb = (srb>>8) & 0x00FF00FF;
    return sag | srb;
}

#define GC(p,ci) (((p)>>((ci)*8))&0xFF)
#define GR(p)    (GC(p,0))
#define GG(p)    (GC(p,1))
#define GB(p)    (GC(p,2))
#define GA(p)    (GC(p,3))
#define SRGBA(r,g,b,a) (((a)<<24) | ((b)<<16) | ((g)<<8) | (r))


extern uint32_t g_frameBuff[N_LAYERS][DISPLAY_WIDTH*DISPLAY_HEIGHT];

extern uint32_t getBlendedPixel( int x, int y );

// SET / GET a single pixel on a layer to a specific RGBA color in the framebuffer
void setPixel( uint8_t layer, uint16_t x, uint16_t y, uint32_t color );
void setPixelColor( uint8_t layer, uint16_t x, uint16_t y, uint8_t cIndex, uint8_t color );
uint32_t getPixel( uint8_t layer, uint16_t x, uint16_t y );

// Draw over a pixel in frmaebuffer at p, color must be premultiplied alpha
void setPixelOver( uint8_t layer, uint16_t x, uint16_t y, uint32_t color );

// Set whole layer to fixed color
extern void setAll( uint8_t layer, uint32_t color );

// write image from a runDmd image file into layer with shades of color r g b
void setFromFile( FILE *f, uint8_t layer, uint32_t color );

// make the layer a little more transparent each call. Factor=255 is strongest. Returns the number of pixels changed.
uint32_t fadeOut( uint8_t layer, uint8_t factor );

void initFb();
void startDrawing( uint8_t layer );
void doneDrawing( uint8_t layer );
void waitDrawingDone();
void doneUpdating();


#endif