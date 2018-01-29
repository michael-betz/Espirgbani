#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H
#include "rgb_led_panel.h"

#define N_LAYERS	3

#define BLEND_LAYERS(t,m,b) ( (t*(255-m)+b*m)/255 )

#define FULL_BLEND_COL(ca,aa,cb,ab) ( (ca*aa+cb*ab*(255-aa)) / (aa+ab*(255-aa)) )
#define FULL_BLEND_A(aa,ab)     ( (aa+ab*(255-aa)) )
#define FULL_BLEND_RGBA(a,b)    SRGBA( 										 \
									FULL_BLEND_COL(GR(a),GA(a),GR(b),GA(b)), \
									FULL_BLEND_COL(GG(a),GA(a),GG(b),GA(b)), \
									FULL_BLEND_COL(GB(a),GA(a),GB(b),GA(b)), \
									FULL_BLEND_A(  GA(a),GA(b)            )  \
								)


// Fast integer `scaling` a*b (255*255=255)
// from http://stereopsis.com/doubleblend.html
// #define INT_MULT(a,b)        ( ((a)+1)*(b) >> 8 )
// #define INT_PRELERP(p, q, a) ( (p) + (q) - INT_MULT(a, p) )

// from http://www.cs.princeton.edu/courses/archive/fall00/cs426/papers/smith95a.pdf
// t = 16 bit temporary variable.
#define INT_MULT(a,b,t) ( (t)=(a)*(b)+0x80, ( (((t)>>8)+(t))>>8 ) )
#define INT_PRELERP(p, q, a, t) ( (p) + (q) - INT_MULT( a, p, t) )
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

#define GR(p) ( p     &0xFF)
#define GG(p) ((p>> 8)&0xFF)
#define GB(p) ((p>>16)&0xFF)
#define GA(p) ((p>>24)&0xFF)
#define SRGBA(r,g,b,a) (((a)<<24) | ((b)<<16) | ((g)<<8) | (r))

extern uint32_t g_frameBuff[N_LAYERS][DISPLAY_WIDTH*DISPLAY_HEIGHT];

extern uint32_t getBlendedPixel( int x, int y );

// Set a single pixel on a layer to a specific RGBA color in the framebuffer
void setPixel( uint8_t layer, uint16_t x, uint16_t y, uint32_t color );

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