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
#include<math.h>
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

int dayBrightness = 70;
int nightBrightness = 2;

void drawTestAnimationFrame(){
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

void aniBackgroundTask(void *pvParameters){
    ESP_LOGI(T,"aniBackgroundTask started");
    uint32_t frameCount = 1;
    uint8_t aniMode = 0;
    while(1){
        switch( aniMode ){
            case 1:
                drawTestAnimationFrame();
                break;
            case 2:
                drawPlasmaFrame();
                break;
            default:
                vTaskDelay( 10 / portTICK_PERIOD_MS );
        }
        updateFrame();
        if( (frameCount%10000) == 0 ){
            aniMode = RAND_AB(0,2);
            if( aniMode == 0 ){
                setAll( 0, 0xFF000000 );
            }
        }
        frameCount++;
        vTaskDelay( 20 / portTICK_PERIOD_MS );
    }
    vTaskDelete( NULL );
}

void seekToFrame( FILE *f, int byteOffset, int frameOffset ){
    byteOffset += DISPLAY_WIDTH * DISPLAY_HEIGHT * frameOffset / 2;
    fseek( f, byteOffset, SEEK_SET );
}

void playAni( FILE *f, headerEntry_t *h ){
    uint32_t aniCol = rand();
    for( int i=0; i<h->nFrameEntries; i++ ){
        frameHeaderEntry_t fh = h->frameHeader[i];
        if( fh.frameId == 0 ){
            startDrawing( 2 );
            setAll( 2, 0xFF000000 );
        } else {
            seekToFrame( f, h->byteOffset+HEADER_SIZE, fh.frameId-1 );
            startDrawing( 2 );
            setFromFile( f, 2, aniCol );
        }
        doneDrawing( 2 );
        // updateFrame();
        vTaskDelay( fh.frameDur / portTICK_PERIOD_MS );
    }
}

void aniClockTask(void *pvParameters){
    time_t now = 0;
    // uint32_t colFill = 0xFF880088;
    struct tm timeinfo;
    timeinfo.tm_min =  0;
    timeinfo.tm_hour= 18;
    char strftime_buf[64];
    srand(time(NULL));
    ESP_LOGI(T,"aniClockTask started");
    while(1){
        if( timeinfo.tm_min==0 ){
            // colFill = rand();
            sprintf( strftime_buf, "/SD/fnt/%d", RAND_AB(0,15) );
            initFont( strftime_buf );
            if( timeinfo.tm_hour>=23 || timeinfo.tm_hour<=7 ){
                brightNessState = BR_NIGHT;
                g_rgbLedBrightness = nightBrightness;
            } else {
                brightNessState = BR_DAY;
                g_rgbLedBrightness = dayBrightness;
            }
        }
        time(&now);       
        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%H:%M", &timeinfo);
        drawStrCentered( strftime_buf, 1, rand(), 0xFF000000 );
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
    setAll( 0, 0xFF220000 );
    setAll( 1, 0xFF002200 );
    setAll( 2, 0xFF000022 );
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
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, 0, 0, 20000/portTICK_PERIOD_MS);
    setAll( 2, 0x00000000 );
    updateFrame();
    vTaskDelay(3000 / portTICK_PERIOD_MS);

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
    ESP_LOGI(T, "Local Time: %s (%ld)", strftime_buf, time(NULL));
    srand(time(NULL));

    initFont( "/SD/fnt/0" );

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
        ESP_LOGD(T, "%s", myHeader.name);
        ESP_LOGD(T, "--------------------------------");
        ESP_LOGD(T, "animationId:       0x%04X", myHeader.animationId);
        ESP_LOGD(T, "nStoredFrames:         %d", myHeader.nStoredFrames);
        ESP_LOGD(T, "byteOffset:    0x%08X", myHeader.byteOffset);
        ESP_LOGD(T, "nFrameEntries:         %d", myHeader.nFrameEntries);
        ESP_LOGD(T, "width:               0x%02X", myHeader.width);
        ESP_LOGD(T, "height:              0x%02X", myHeader.height);
        ESP_LOGD(T, "unknowns:            0x%02X", myHeader.unknown0 );
        ESP_LOG_BUFFER_HEX_LEVEL( T, myHeader.unknown1, sizeof(myHeader.unknown1), ESP_LOG_DEBUG );
        ESP_LOGD(T, "frametimes:");
        ESP_LOG_BUFFER_HEX_LEVEL( T, myHeader.frameHeader, myHeader.nFrameEntries*2, ESP_LOG_DEBUG );
        playAni( f, &myHeader );
        free( myHeader.frameHeader );
        myHeader.frameHeader = NULL;
        // Keep a single frame displayed for a bit
        if( myHeader.nStoredFrames<=3 || myHeader.nFrameEntries<=3 ) vTaskDelay( 3000/portTICK_PERIOD_MS );
        // // Fade out the frame
        uint32_t nTouched = 1;
        while( nTouched ){
            startDrawing( 2 );
            nTouched = fadeOut( 2, RAND_AB(1,50) );
            doneDrawing( 2 );
            // updateFrame();
            vTaskDelay( 20 / portTICK_PERIOD_MS);
        }
        startDrawing( 2 );
        setAll( 2, 0x00000000 );    //Make layer fully transparent
        doneDrawing( 2 );
        vTaskDelay(15000 / portTICK_PERIOD_MS);
    }
    fclose(f);
}
