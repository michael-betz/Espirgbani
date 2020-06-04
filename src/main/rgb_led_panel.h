#ifndef RGB_LED_PANEL_H
#define RGB_LED_PANEL_H

//Width of the chain of panels (shift registers) [pixels]
#define DISPLAY_WIDTH  128
#define DISPLAY_HEIGHT  32

//My display has each row swapped with its neighbour (so the rows are 2-1-4-3-6-5-8-7-...). If your display
//is more sane, uncomment this to get a good image.
// #define DISPLAY_ROWS_SWAPPED 1

//This is the bit depth, per RGB subpixel, of the data that is sent to the display.
//The effective bit depth (in computer pixel terms) is less because of the PWM correction. With
//a bitplane count of 7, you should be able to reproduce an 16-bit image more or less faithfully, though.
#define BITPLANE_CNT 7

//x*DISPLAY_HEIGHT RGB leds, 2 pixels per 16-bit value...
#define BITPLANE_SZ (DISPLAY_WIDTH*DISPLAY_HEIGHT/2)	//[16 bit words]

// GPIO definitions (change your pinout)
// Upper half RGB
#define GPIO_R1      2
#define GPIO_G1     15
#define GPIO_B1     13
// Lower half RGB
#define GPIO_R2     16
#define GPIO_G2     27
#define GPIO_B2     17
// Control signals
#define GPIO_A       5
#define GPIO_B      18
#define GPIO_C      19
#define GPIO_D      23
#define GPIO_LAT    26
#define GPIO_BLANK  25
#define GPIO_CLK    22

// -------------------------------------------
//  Meaning of the bits in a 16 bit DMA word
// -------------------------------------------
//Upper half RGB
#define BIT_R1 (1<<0)
#define BIT_G1 (1<<1)
#define BIT_B1 (1<<2)
//Lower half RGB
#define BIT_R2 (1<<3)
#define BIT_G2 (1<<4)
#define BIT_B2 (1<<5)
#define BIT_A (1<<6)
#define BIT_B (1<<7)
#define BIT_C (1<<8)
#define BIT_D (1<<9)
#define BIT_LAT (1<<10)
#define BIT_BLANK (1<<11)
// -1 = don't care
// -1
// -1
// -1

//Change to set the global brightness of the display, range 0 ... (DISPLAY_WIDTH-8)
extern int g_rgbLedBrightness;

void init_rgb();
void updateFrame();

#endif
