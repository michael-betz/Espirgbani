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

#define P_SIZE 32
static const uint8_t palette_hot[P_SIZE][3] = {
    {  0,   0,   0},
    { 31,   0,   0},
    { 52,   0,   0},
    { 73,   0,   0},
    { 97,   0,   0},
    {118,   0,   0},
    {139,   0,   0},
    {160,   0,   0},
    {183,   0,   0},
    {204,   0,   0},
    {225,   0,   0},
    {246,   0,   0},
    {255,  15,   0},
    {255,  36,   0},
    {255,  57,   0},
    {255,  78,   0},
    {255, 102,   0},
    {255, 123,   0},
    {255, 144,   0},
    {255, 165,   0},
    {255, 188,   0},
    {255, 209,   0},
    {255, 230,   0},
    {255, 251,   0},
    {255, 255,  30},
    {255, 255,  62},
    {255, 255,  93},
    {255, 255, 125},
    {255, 255, 160},
    {255, 255, 191},
    {255, 255, 223},
    {255, 255, 255}
};

static const uint8_t palette_gnu[P_SIZE][3] = {
    {  0,   0,   0},
    {  0,   0,  32},
    {  0,   0,  64},
    {  0,   0,  96},
    {  0,   0, 131},
    {  0,   0, 163},
    {  0,   0, 195},
    {  0,   0, 227},
    {  7,   0, 255},
    { 32,   0, 255},
    { 57,   0, 255},
    { 82,   0, 255},
    {110,   0, 255},
    {135,   0, 255},
    {160,  15, 239},
    {185,  31, 223},
    {213,  49, 205},
    {238,  65, 189},
    {255,  81, 173},
    {255,  97, 157},
    {255, 115, 139},
    {255, 131, 123},
    {255, 147, 107},
    {255, 163,  91},
    {255, 181,  73},
    {255, 197,  57},
    {255, 213,  41},
    {255, 229,  25},
    {255, 247,   7},
    {255, 255,  54},
    {255, 255, 154},
    {255, 255, 255}
};

static uint8_t pbuff[DISPLAY_WIDTH * (DISPLAY_HEIGHT + 1)];

void flameSeedRow() {
    int c = 0, v = 0;
    // ESP_LOGI(T, "flameseed");
    for (unsigned x=0; x<DISPLAY_WIDTH; x++) {
        if (c == 0){
            c = RAND_AB(1, 10);
            v = (c - 1) * 3;
        }
        pbuff[x + DISPLAY_HEIGHT * DISPLAY_WIDTH] = v;
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
    const uint8_t *colors;
    if ((frm % 100) == 0)
        flameSeedRow();

    for (int y=DISPLAY_HEIGHT; y>0; y--)
        for (int x=0; x<DISPLAY_WIDTH; x++)
            flameSpread(x + y * DISPLAY_WIDTH);

    startDrawing(0);
    for (unsigned y=0; y<DISPLAY_HEIGHT; y++){
        for (unsigned x=0; x<DISPLAY_WIDTH; x++) {
            uint8_t pInd = pbuff[x + y * DISPLAY_WIDTH];
            if ((frm >> 12) & 1)
                colors = palette_gnu[pInd];
            else
                colors = palette_hot[pInd];
            setPixel(0, x, y, SRGBA(colors[0], colors[1], colors[2], 0xFF));
        }
    }
    doneDrawing(0);

    frm++;
}

void aniBackgroundTask(void *pvParameters){
    ESP_LOGI(T,"aniBackgroundTask started");
    uint32_t frameCount = 1;
    uint8_t aniMode = 0;
    setAll(0, 0xFF000000);
    while(1){
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
            default:
                vTaskDelay( 10 / portTICK_PERIOD_MS );
        }
        updateFrame();
        if ((frameCount % 10000) == 0){
            aniMode = RAND_AB(0, 6);
            if(aniMode == 0 || aniMode > 4){
                int tempColor = 0xFF000000;// | scale32( 128, rand() );
                setAll(0, tempColor);
            }
        }
        frameCount++;
        vTaskDelay( 10 / portTICK_PERIOD_MS );
    }
    vTaskDelete( NULL );
}
