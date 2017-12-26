// Copyright 2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "esp_log.h"
#include "wifi_startup.h"
#include "web_console.h"
#include "libnsgif/libnsgif.h"
#include "rgb_led_panel.h"
#include "sdcard.h"
#include "animations.h"
#include "frame_buffer.h"
#include "bmp.h"
#include "font.h"
#include "app_main.h"

static const char *T = "MAIN_APP";

void drawTestAnimationFrame(){
    static int frm=1;
    static uint8_t aniZoom=0x04, boost=7;
    startDrawing( 0 );
    for( int y=0; y<=31; y++ )
        for( int x=0; x<=127; x++ )
            setPixel( 0, x, y, ((x+y+frm)&aniZoom)*boost, ((x-y-frm)&aniZoom)*boost, ((x^y)&aniZoom)*boost, 0 );
    doneDrawing( 0 );
    if( (frm%1024) == 0 ){
        if( aniZoom == 0xFF ){
            aniZoom = 0x04;
            boost=8;
        } else {
            aniZoom = (aniZoom<<1)|1;
            boost = (boost>>1) | 1;
        }
    }
    frm++;
}

void seekToFrame( FILE *f, int byteOffset, int frameOffset ){
    byteOffset += DISPLAY_WIDTH * DISPLAY_HEIGHT * frameOffset / 2;
    fseek( f, byteOffset, SEEK_SET );
}

void playAni( FILE *f, headerEntry_t *h ){
    uint8_t r, g, b;
    r = rand()|0x1F;
    g = rand()|0x1F;
    b = rand()|0x1F;
    for( int i=0; i<h->nFrameEntries; i++ ){
        frameHeaderEntry_t fh = h->frameHeader[i];
        if( fh.frameId == 0 ){
            startDrawing( 2 );
            setAll( 2, 0, 0, 0, 0 );
        } else {
            seekToFrame( f, h->byteOffset+HEADER_SIZE, fh.frameId-1 );
            startDrawing( 2 );
            setFromFile( f, 2, r, g, b );
        }
        doneDrawing( 2 );
        // updateFrame();
        vTaskDelay( fh.frameDur / portTICK_PERIOD_MS );
    }
}

// Random number within the range [a,b]
#define RAND_AB(a,b) (rand()%(b+1-a)+a)

enum {ANI_DMD, ANI_PATT} aniState;

void aniBackgroundTask(void *pvParameters){
    ESP_LOGI(T,"aniBackgroundTask started");
    while(1){
        drawTestAnimationFrame();
        updateFrame();
        vTaskDelay( 20 / portTICK_PERIOD_MS );
    }
    vTaskDelete( NULL );
}

void aniClockTask(void *pvParameters){
    time_t now = 0;
    struct tm timeinfo = { 0 };
    char strftime_buf[64];
    ESP_LOGI(T,"aniClockTask started");
    while(1){
        time(&now);       
        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%H:%M", &timeinfo);
        startDrawing( 1 );
        setAll( 1, 0, 0, 0, 0xFF );
        uint16_t w = getStrWidth( strftime_buf );
        drawStr( strftime_buf, (DISPLAY_WIDTH-w)/2, 0, 1, 0x00, 0x00, 0x00 );
        doneDrawing( 1 );
        // updateFrame();
        vTaskDelay( 1000*60 / portTICK_PERIOD_MS );
    }
    vTaskDelete( NULL );
}

void app_main(){
    //------------------------------
    // Enable RAM log file
    //------------------------------
    esp_log_set_vprintf( wsDebugPrintf );

    //------------------------------
    // Init rgb tiles
    //------------------------------
    init_rgb();
    setAll( 0, 0x22, 0x00, 0x00, 0 );
    setAll( 1, 0x00, 0x22, 0x00, 0 );
    setAll( 2, 0x00, 0x00, 0x22, 0 );
    updateFrame();
    g_rgbLedBrightness = 70;

    //------------------------------
    // Init filesystems
    //------------------------------
    initSpiffs();
    initSd();

    //------------------------------
    // Startup wifi & webserver
    //------------------------------
    ESP_LOGI(T,"Starting network infrastructure ...");
    wifi_conn_init();
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    //------------------------------
    // Set the clock / print time
    //------------------------------
    // Set timezone to Eastern Standard Time and print local time
    setenv("TZ", "PST8PDT", 1);
    tzset();
    time_t now = 0;
    struct tm timeinfo = { 0 };
    char strftime_buf[64];
    time(&now);       
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(T, "Local Time: %s", strftime_buf);
    srand(time(NULL));

    initFont( "/SD/ubuntu" );

    // //------------------------------
    // // Startup animated background layer
    // //------------------------------
    xTaskCreate(&aniBackgroundTask, "aniBackground", 4096, NULL, 0, NULL);

    //------------------------------
    // Startup clock layer
    //------------------------------
    xTaskCreate(&aniClockTask, "aniClock", 4096, NULL, 0, NULL);

    //------------------------------
    // Read animation file from SD
    //------------------------------
    FILE *f = fopen( "/SD/runDmd.img", "r" );
    fileHeader_t fh;
    getFileHeader( f, &fh );
    printf("Build: %s, Found %d animations\n\n", fh.buildStr, fh.nAnimations);
    fseek( f, HEADER_OFFS, SEEK_SET );
    headerEntry_t myHeader;

    int aniId;
    while(1){
        aniId = RAND_AB( 0, fh.nAnimations-1 );
        readHeaderEntry( f, &myHeader, aniId );
        ESP_LOGI(T, "%s", myHeader.name);
        ESP_LOGI(T, "--------------------------------");
        ESP_LOGI(T, "animationId:       0x%04X", myHeader.animationId);
        ESP_LOGI(T, "nStoredFrames:         %d", myHeader.nStoredFrames);
        ESP_LOGI(T, "byteOffset:    0x%08X", myHeader.byteOffset);
        ESP_LOGI(T, "nFrameEntries:         %d", myHeader.nFrameEntries);
        ESP_LOGI(T, "width:               0x%02X", myHeader.width);
        ESP_LOGI(T, "height:              0x%02X", myHeader.height);
        ESP_LOGI(T, "unknowns:            0x%02X", myHeader.unknown0 );
        ESP_LOG_BUFFER_HEX( T, myHeader.unknown1, sizeof(myHeader.unknown1) );
        ESP_LOGI(T, "frametimes:");
        ESP_LOG_BUFFER_HEX( T, myHeader.frameHeader, myHeader.nFrameEntries*2 );
        playAni( f, &myHeader );
        free( myHeader.frameHeader );
        myHeader.frameHeader = NULL;
        vTaskDelay( 1000 / portTICK_PERIOD_MS);
        startDrawing( 2 );
        setAll( 2, 0, 0, 0, 0xFF );
        doneDrawing( 2 );
        vTaskDelay(15000 / portTICK_PERIOD_MS);
        ESP_LOGI(T, "\n");
    }
    fclose(f);
}
