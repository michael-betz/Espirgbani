#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "esp_log.h"
#include "web_console.h"
#include "esp_heap_caps.h"
#include "i2s_parallel.h"
#include "frame_buffer.h"
#include "rgb_led_panel.h"
#include "assert.h"
#include "wifi_startup.h"


static const char *T = "RGB_LED_PANEL";

int g_rgbLedBrightness = 1;
//which buffer is the backbuffer, as in, which one is not active so we can write to it
int backbuf_id = 0;
//Internal double buffered array of bitPLanes
uint16_t *bitplane[2][BITPLANE_CNT];
//DISPLAY_WIDTH * 32 * 3 array with image data, 8R8G8B

// .json configurable parameters
static cJSON *jPanel = NULL;
static int latch_offset = 0;
static unsigned extra_blank = 0;
static bool isColumnSwapped = false;

void init_rgb(){
    initFb();
    i2s_parallel_buffer_desc_t bufdesc[2][1<<BITPLANE_CNT];
    i2s_parallel_config_t cfg = {
        .gpio_bus={GPIO_R1, GPIO_G1, GPIO_B1,   GPIO_R2, GPIO_G2, GPIO_B2,   -1, -1,   GPIO_A, GPIO_B, GPIO_C, GPIO_D,   GPIO_LAT, GPIO_BLANK,   -1, -1},
        .gpio_clk=GPIO_CLK,
        .bits=I2S_PARALLEL_BITS_16,
        .bufa=bufdesc[0],
        .bufb=bufdesc[1],
    };

    // get `panel` dictionary of json file
    jPanel = jGet(getSettings(), "panel");
    if (jPanel) {
        // Swap pixel x[0] with x[1]
        isColumnSwapped = jGetI(jPanel, "column_swap") > 0;
        if (isColumnSwapped)
            ESP_LOGW(T, "column_swap applied!");

        // adjust clock cycle of the latch pulse (nominally = 0 = last pixel)
        latch_offset = ((DISPLAY_WIDTH - 1) + jGetI(jPanel, "latch_offset")) % DISPLAY_WIDTH;
        ESP_LOGW(T, "latch_offset = %d", latch_offset);

        // adjust extra blanking cycles to reduce ghosting effects
        extra_blank = jGetI(jPanel, "extra_blank");

        // set clock divider
        cfg.clk_div = jGetI(jPanel, "clkm_div_num");
        if (cfg.clk_div < 1) cfg.clk_div = 1;
        ESP_LOGW(T, "clkm_div_num = %d", cfg.clk_div);

        cfg.is_clk_inverted = jGetI(jPanel, "is_clk_inverted") > 0;
    } else {
        ESP_LOGE(T,".json: could not read `panel` section");
        cfg.clk_div = 16;     // = 2.4 MHz
        cfg.is_clk_inverted = true;
        latch_offset = DISPLAY_WIDTH - 1;
    }

    for (int i=0; i<BITPLANE_CNT; i++) {
        for (int j=0; j<2; j++) {
            bitplane[j][i] = heap_caps_malloc(BITPLANE_SZ*2, MALLOC_CAP_DMA);
            assert(bitplane[j][i] && "Can't allocate bitplane memory");
            memset(bitplane[j][i], 0, BITPLANE_SZ*2);
        }
    }

    //Do binary time division setup. Essentially, we need n of plane 0, 2n of plane 1, 4n of plane 2 etc, but that
    //needs to be divided evenly over time to stop flicker from happening. This little bit of code tries to do that
    //more-or-less elegantly.
    int times[BITPLANE_CNT]={0};
    for (int i=0; i<((1<<BITPLANE_CNT)-1); i++) {
        int ch=0;
        //Find plane that needs insertion the most
        for (int j=0; j<BITPLANE_CNT; j++) {
            if (times[j]<=times[ch]) ch=j;
        }
        //Insert the plane
        for (int j=0; j<2; j++) {
            bufdesc[j][i].memory=bitplane[j][ch];
            bufdesc[j][i].size=BITPLANE_SZ*2;
        }
        //Magic to make sure we choose this bitplane an appropriate time later next time
        times[ch]+=(1<<(BITPLANE_CNT-ch));
    }

    //End markers
    bufdesc[0][((1<<BITPLANE_CNT)-1)].memory = NULL;
    bufdesc[1][((1<<BITPLANE_CNT)-1)].memory = NULL;

    //Setup I2S
    i2s_parallel_setup(&I2S1, &cfg);

    ESP_LOGI(T,"I2S setup done.");
}

void updateFrame(){
    // Wait until all the layers are done updating
    waitDrawingDone();
    //Fill bitplanes with the data for the current image from frameBuffer
    for (int pl=0; pl<BITPLANE_CNT; pl++) {
        int mask=(1<<(8-BITPLANE_CNT+pl)); //bitmask for pixel data in input for this bitplane
        uint16_t *p=bitplane[backbuf_id][pl]; //bitplane location to write to
        for (unsigned int y=0; y<16; y++) {
            int lbits=0;                //Precalculate line bits of the *previous* line, which is the one we're displaying now
            if ((y-1)&1) lbits|=BIT_A;
            if ((y-1)&2) lbits|=BIT_B;
            if ((y-1)&4) lbits|=BIT_C;
            if ((y-1)&8) lbits|=BIT_D;
            for (int fx=0; fx<DISPLAY_WIDTH; fx++) {
                int x = fx;

                if (isColumnSwapped)
                    x ^= 1;

                int v=lbits;
                // Do not show image while the line bits are changing
                if (fx < extra_blank || fx >= g_rgbLedBrightness + 8) v |= BIT_BLANK;

                // latch on last bit...
                if (fx == latch_offset) v |= BIT_LAT;

                int c1 = getBlendedPixel(x, y);
                int c2 = getBlendedPixel(x, y+16);
                if (c1 & (mask<<16)) v|=BIT_R1;
                if (c1 & (mask<<8)) v|=BIT_G1;
                if (c1 & (mask<<0)) v|=BIT_B1;
                if (c2 & (mask<<16)) v|=BIT_R2;
                if (c2 & (mask<<8)) v|=BIT_G2;
                if (c2 & (mask<<0)) v|=BIT_B2;

                //Save the calculated value to the bitplane memory
                *p++=v;
            }
        }
    }

    doneUpdating();
    //Show our work (on next occasion)
    i2s_parallel_flip_to_buffer(&I2S1, backbuf_id);
    //Switch bitplane buffers
    backbuf_id ^= 1;
}
