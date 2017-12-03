#ifndef RGB_LED_PANEL_H
#define RGB_LED_PANEL_H

//Width of the chain of panels (shift registers) [pixels]
#define DISPLAY_WIDTH 128

//This is the bit depth, per RGB subpixel, of the data that is sent to the display.
//The effective bit depth (in computer pixel terms) is less because of the PWM correction. With
//a bitplane count of 7, you should be able to reproduce an 16-bit image more or less faithfully, though.
#define BITPLANE_CNT 6

//x*32 RGB leds, 2 pixels per 16-bit value...
#define BITPLANE_SZ (DISPLAY_WIDTH*32/2)	//[16 bit words]

// GPIO definitions (change your pinout)
#define GPIO_A       5
#define GPIO_B      18
#define GPIO_C      19
#define GPIO_D      23 //21
#define GPIO_OE     25
#define GPIO_LAT    26
#define GPIO_R0      2
#define GPIO_G0     15
#define GPIO_B0     13 //4
#define GPIO_R1     16
#define GPIO_G1     27
#define GPIO_B1     17
#define GPIO_CLK    22

//Bit definitions
#define BIT_A   (1<< 0)
#define BIT_B   (1<< 1) 
#define BIT_C   (1<< 2) 
#define BIT_D   (1<< 3) 
#define BIT_OE  (1<< 4) 
#define BIT_LAT (1<< 5) 
//Upper half RGB
#define BIT_R0  (1<< 6)
#define BIT_G0  (1<< 7) 
#define BIT_B0  (1<< 8)
//Lower half RGB
#define BIT_R1  (1<< 9) 
#define BIT_G1  (1<<10) 
#define BIT_B1  (1<<11) 

//Change to set the global brightness of the display, range 1 ... (DISPLAY_WIDTH-2)
extern int g_rgbLedBrightness;

void init_rgb();
void updateFrame();
void setpixel( int x, int y, uint8_t r, uint8_t g, uint8_t b );

#endif