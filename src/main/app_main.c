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
#include "esp_log.h"
#include "wifi_startup.h"
#include "web_console.h"
#include "libnsgif/libnsgif.h"
#include "rgb_led_panel.h"
#include "sdcard.h"
#include "app_main.h"

static const char *T = "MAIN_APP";

void app_main(){
    //------------------------------
    // Enable RAM log file
    //------------------------------
    esp_log_set_vprintf( wsDebugPrintf );
    //------------------------------
    // Init rgb tiles
    //------------------------------
    init_rgb();
    g_rgbLedBrightness = 64;
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


    // gif_bitmap_callback_vt bitmap_callbacks = {
    //     bitmap_create,
    //     bitmap_destroy,
    //     bitmap_get_buffer,
    //     bitmap_set_opaque,
    //     bitmap_test_opaque,
    //     bitmap_modified
    // };
    // gif_animation gif;
    // /* create our gif animation */
    // gif_create(&gif, &bitmap_callbacks);


    int frm=1;
    uint8_t aniZoom = 0x04;
    while(1){
        for( int y=0; y<=31; y++ )
            for( int x=0; x<=127; x++ )
                setpixel(x, y, (x+y+frm)&aniZoom, (x-y-frm)&aniZoom, (x^y)&aniZoom);
        if( (frm%1024) == 0 ){
            if( aniZoom == 0xFF ){
                aniZoom = 0x04;
            } else {
                aniZoom = (aniZoom<<1)|1;
            }
        }

        // for( int x=0; x<=127; x++ ){
        //     setpixel(x, 0, 0xFF, 0xFF, 0xFF);
        //     setpixel(x,31, 0xFF, 0xFF, 0xFF);
        // }
        // for( int y=0; y<=31; y++ ){
        //     setpixel(0,   y, 0xFF, 0xFF, 0xFF);
        //     setpixel(127, y, 0xFF, 0xFF, 0xFF);
        // }
        updateFrame();
        vTaskDelay(40 / portTICK_PERIOD_MS); //animation has an 100ms interval        
        frm++;
    }

}
