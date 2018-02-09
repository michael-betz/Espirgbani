# Pinball DMD clock

![clock](https://github.com/yetifrisstlama/Espirgbani/raw/master/pcb/front.jpg)

Work in progress. [Some writeup here](http://yetifrisstlama.blogspot.com/2018/02/the-esp32-pinball-rgb-matrix-animation.html). Based on ESP32 and sprite_tm's I2C DMA driver.

# PCB
Fits inside one of the 64x32 panels.
![front](https://github.com/yetifrisstlama/Espirgbani/raw/master/pcb/1.png)

![back](https://github.com/yetifrisstlama/Espirgbani/raw/master/pcb/2.png)

![wiring](https://github.com/yetifrisstlama/Espirgbani/raw/master/pcb/back.jpg)

## TODO
  * Change programming port from custom pogo-pins to a normal header
  * Add an ambient light level sensor
  * Re-arrange the mounting holes
  
# SD card instructions
Format as FAT32, then copy the following files: 
  * `./knownWifis.json`
  * `./runDmd.img`
  * `./fnt/{d}.fnt`
  * `./fnt/{d}_0.bmp`
  
## example `knownWifis.json`
```json
{
  "<ssid>": {
    "pw":   "<password>",
    "type": "full"
  }
}
```

## how to get `runDmd.img`
  * Find a run-dmd image file and write it to SD card according to their instructions (HDD Raw Copy Tool and Windows required)
  * In linux, dump the first few hundred megabytes of the SD card to a file: 
  ```bash
  sudo dd ...
  ```
  
## how to get the font `.bmp` and `.fnt` files
  * Find some nice fonts which look good at small size and copy the `.ttf` files into the git tree at `./dev/fonts`
  * Use [bmFont](http://www.angelcode.com/products/bmfont/) (works well with wine in linux) to generate the bitmap fonts. Good settings are provided in `dev/clockFont.bmfc`
    * .png output file
    * .fnt binary file
    * 1 pixel backed in outline
    * 32 bit per pixel (G = filling, B = outline)
  * Use imagemagick to convert all image files to bmp format
  * Rename them to 0_0.bmp, 0.fnt;  1_0.bmp, 1.fnt, ...
  * Alternatively, run `./dev/generateFonts.sh` which should automate these steps
