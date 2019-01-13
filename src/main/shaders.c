// The animated background patterns

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "esp_log.h"
// #include "wifi_startup.h"
// #include "web_console.h"
#include "rgb_led_panel.h"
// #include "sdcard.h"
// #include "animations.h"
#include "frame_buffer.h"
// #include "bmp.h"
// #include "font.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "shaders.h"
#include "app_main.h"
#include "palette.h"

static const char *T = "SHADERS";

void drawXorFrame(){
    static int frm=0;
    static uint16_t aniZoom=0x04, boost=7;
    startDrawing( 0 );
    for( int y=0; y<=31; y++ )
        for( int x=0; x<=127; x++ )
            setPixel( 0, x, y, SRGBA( ((x+y+frm)&aniZoom)*boost, ((x-y-frm)&aniZoom)*boost, ((x^y)&aniZoom)*boost, 0xFF ) );
    doneDrawing( 0 );
    if( (frm%1024) == 0 ){
        aniZoom = rand();
        boost   = RAND_AB(1,8);
        ESP_LOGI(T, "aniZoom = %d,  boost = %d", aniZoom, boost);
    }
    frm++;
}

void drawPlasmaFrame(){
    static int frm=3001, i, j, k, l;
    int temp1, temp2;
    if( frm > 3000 ){
        frm = 0;
        i = RAND_AB(1,8);
        j = RAND_AB(1,8);
        k = ((i<<5)-1);
        l = ((j<<5)-1);
    }
    startDrawing( 0 );
    for( int y=0; y<=31; y++ ){
        for( int x=0; x<=127; x++ ){
            temp1 = abs((( i*y+(frm*16)/(x+16))%64)-32)*7;
            temp2 = abs((( j*x+(frm*16)/(y+16))%64)-32)*7;
            setPixel( 0, x, y, SRGBA( temp1&k, temp2&l, (temp1^temp2)&0x88, 0xFF ) );
        }
    }
    doneDrawing( 0 );
    frm++;
}

void drawAlienFlameFrame(){
    //  *p points to last pixel of second last row (lower right)
    // uint32_t *p = (uint32_t*)g_frameBuff[0][DISPLAY_WIDTH*(DISPLAY_HEIGHT-1)-1];
    // uint32_t *tempP;
    uint32_t colIndex, temp;
    startDrawing( 0 );
    colIndex = RAND_AB(0,127);
    temp = getPixel( 0, colIndex, 31 );
    setPixel( 0, colIndex, 31, 0xFF000000 | (temp+2) );
    colIndex = RAND_AB(0,127);
    temp = getPixel( 0, colIndex, 31 );
    temp = scale32( 127, temp );
    setPixel( 0, colIndex, 31, temp );
    for( int y=30; y>=0; y-- ){
        for( int x=0; x<=127; x++ ){
            colIndex = RAND_AB(0,2);
            temp  = GC(getPixel(0, x-1, y+1), colIndex);
            temp += GC(getPixel(0, x,   y+1), colIndex);
            temp += GC(getPixel(0, x+1, y+1), colIndex);
            setPixelColor( 0, x, y, RAND_AB(0,2), MIN(temp*5/8,255) );
        }
    }
    doneDrawing( 0 );
}

static uint8_t pbuff[DISPLAY_WIDTH * (DISPLAY_HEIGHT + 1)];

void flameSeedRow() {
    int c = 0, v = 0;
    uint8_t *p = &pbuff[DISPLAY_HEIGHT * DISPLAY_WIDTH];
    // ESP_LOGI(T, "flameseed");
    for (unsigned x=0; x<DISPLAY_WIDTH; x++) {
        if (c <= 0){
            c = RAND_AB(0, 5);     // width
            v = RAND_AB(0, 2) - 1; // intensity -1 .. 1
        }
        *p += v;
        // Detect negative underflow
        if (*p > 128)
            *p = 0;
        // Detect positive overflow
        else if (*p >= P_SIZE)
            *p = P_SIZE - 1;
        p++;
        c -= 1;
    }
}

void flameSpread(unsigned ind) {
    uint8_t pixel = pbuff[ind];
    if (pixel == 0) {
        pbuff[ind - DISPLAY_WIDTH] = 0;
    } else {
        unsigned r = RAND_AB(0, 2);
        int tmp = pixel - r;
        if (tmp < 0) tmp = 0;
        if (tmp >= P_SIZE) tmp = P_SIZE - 1;
        pbuff[ind - DISPLAY_WIDTH - (r & 1)] = tmp;
    }
}

void drawDoomFlameFrame() {
    static unsigned frm = 0;
    static const uint32_t *pal = g_palettes[0];
    // Slowly modulate flames / change palette now and then
    if ((frm % 25) == 0){
        flameSeedRow();
        if ((frm % 9000) == 0) {
            pal = get_random_palette();
        }
    }
    // Flame generation
    for (int y=DISPLAY_HEIGHT; y>0; y--)
        for (int x=0; x<DISPLAY_WIDTH; x++)
            flameSpread(x + y * DISPLAY_WIDTH);
    // Colorize flames and write into framebuffer
    startDrawing(0);
    for (unsigned y=0; y<DISPLAY_HEIGHT; y++){
        for (unsigned x=0; x<DISPLAY_WIDTH; x++) {
            uint8_t pInd = pbuff[x + y * DISPLAY_WIDTH];
            setPixel(0, x, y, pal[pInd % P_SIZE]);
        }
    }
    doneDrawing(0);
    frm++;
}

void aniBackgroundTask(void *pvParameters){
    ESP_LOGI(T,"aniBackgroundTask started");
    uint32_t frameCount = 1;
    uint8_t aniMode = 0;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    setAll(0, 0xFF000000);
    while(1){
        if ((frameCount % 10000) == 0){
            aniMode = RAND_AB(0, 6);
            if(aniMode == 0 || aniMode > 4){
                int tempColor = 0xFF000000;// | scale32(128, rand());
                setAll(0, tempColor);
            }
        }
        switch(aniMode){
            case 1:
                drawXorFrame();
                break;
            case 2:
                drawPlasmaFrame();
                break;
            case 3:
                drawAlienFlameFrame();
                break;
            case 4:
                drawDoomFlameFrame();
                break;
        }
        vTaskDelayUntil(&xLastWakeTime, 10 / portTICK_PERIOD_MS);
        updateFrame();
        frameCount++;
    }
    vTaskDelete( NULL );
}
