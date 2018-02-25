# Pinball DMD clock

![clock](https://github.com/yetifrisstlama/Espirgbani/raw/master/pcb/front.jpg)

Work in progress. [Some writeup here](http://yetifrisstlama.blogspot.com/2018/02/the-esp32-pinball-rgb-matrix-animation.html). Based on ESP32 and sprite_tm's I2C DMA driver.

# PCB
Fits inside one of the 64x32 panels.

![back](https://github.com/yetifrisstlama/Espirgbani/raw/master/pcb/1.png)

![wiring](https://github.com/yetifrisstlama/Espirgbani/raw/master/pcb/back.jpg)

### Revisions
Changes included in Rev 1
  * Change programming port from custom pogo-pins to a normal header
  * Add an ambient light level sensor
  * [ Re-arrange the mounting hole ] TODO
  
# SD card instructions
Format as FAT32, then copy the following files: 
  * `./settings.json`
  * `./runDmd.img`
  * `./fnt/{d}.fnt`
  * `./fnt/{d}_0.bmp`
  
## example `settings.json`
```json
{
    "power": {
        "hi": {
            "h": 7,
            "m": 30,
            "p": 40,
            "pingIp": "192.168.1.208"
        },
        "lo": {
            "h": 23,
            "m": 0,
            "p": 1
        }
    },
    "hostname": "espirgbani",
    "wifis": {
        "<wifi-ssid>": {
            "pw": "<wifi-password>"
        }
    }
}
```
__Some notes about the `power` section__
  * Switches to brightness `40` at `7:30`
  * But only if `192.168.1.208` responds to pings
  * Switches to brightness `1` at `23:00`
  
Note that, once running, the settings.json file can be edited through the web interface.

## how to get `runDmd.img`
  * Find a run-dmd image file and write it to SD card according to their instructions (HDD Raw Copy Tool and Windows required)
  * In linux, dump the first few hundred megabytes of the SD card to a file: 
  ```bash
  sudo dd if=/dev/sdb of=./runDmd.img bs=1M count=125
  ```
  
## how to get the font `.bmp` and `.fnt` files
  * Find some nice fonts which look good at small size and copy the `.ttf` files into the git tree at `./dev/fonts`
  * Use [bmFont](http://www.angelcode.com/products/bmfont/) (works well with wine in linux) to generate the bitmap fonts. Good settings are provided in `dev/clockFont.bmfc`
    * Width: 52, Height: 2000
    * 32 bit per pixel (A = one, R = glyph, G = outline, B = zero)
    * .png texture file
    * binary .fnt descriptor file
    * Size: 30 px, Outline thickness: 1 px
  * Use imagemagick to convert all image files to bmp format
  * Rename them to 0_0.bmp, 0.fnt;  1_0.bmp, 1.fnt, ...
  * Alternatively, run `./dev/generateFonts.sh` which should automate these steps
