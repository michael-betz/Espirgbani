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
#include "rgb_led_panel.h"
#include "app_main.h"

static const char *T = "MAIN_APP";

void app_main(){
    //------------------------------
    // Enable RAM log file
    //------------------------------
    initFs();
    esp_log_set_vprintf( wsDebugPrintf );
    //------------------------------
    // Init rgb tiles
    //------------------------------
    init_rgb();
    //------------------------------
    // Startup wifi & webserver
    //------------------------------
    ESP_LOGI(T,"Starting network infrastructure ...");
    wifi_conn_init();

    int frm=0;
    while(1){
        for( int y=0; y<=31; y++ )
            for( int x=0; x<=127; x++ )
                setpixel(x, y, (x+y+frm)&0x2F, (x-y-frm)&0x2F, (x*y)&0x2F);

        for( int x=0; x<=127; x++ ){
            setpixel(x, 0, 5, 0, 0);
            setpixel(x,31, 5, 0, 0);
        }
        for( int y=0; y<=31; y++ ){
            setpixel(0,   y, 0, 5, 0);
            setpixel(127, y, 0, 5, 0);
        }
        g_rgbLedBrightness = 32;
        updateFrame();
        vTaskDelay(50 / portTICK_PERIOD_MS); //animation has an 100ms interval        
        frm++;
    }

}
