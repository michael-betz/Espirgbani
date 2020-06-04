# Pinball DMD clock

![clock](https://github.com/yetifrisstlama/Espirgbani/raw/master/pcb/pdf/front.jpg)

Work in progress. [Some writeup here](http://yetifrisstlama.blogspot.com/2018/02/the-esp32-pinball-rgb-matrix-animation.html). Based on ESP32 and sprite_tm's I2C DMA driver.

# PCB
Fits inside one of the 64x32 panels.

![back](https://github.com/yetifrisstlama/Espirgbani/raw/master/pcb/pdf/1.png)

![wiring](https://github.com/yetifrisstlama/Espirgbani/raw/master/pcb/pdf/back.jpg)

### Revisions
Changes included in Rev 1
  * Change programming port from custom pogo-pins to a normal header
  * Add input for ambient light level sensor
  * Re-arrange the mounting holes
  * Add second footprint for a uSD card

# Building
Unfortunately the projects which I based this software on are either completely obsolete or they moved on a lot. Building still works (as of April 2020) but only when using exactly the versions and git commits as specified below.

install [version 5.2.0 of `xtensa-esp32-elf` toolchain](https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz) and put it in your `PATH`.

```bash
$ xtensa-esp32-elf-gcc --version
xtensa-esp32-elf-gcc (crosstool-NG crosstool-ng-1.22.0-80-g6c4433a) 5.2.0
Copyright (C) 2015 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

install the esp-idf library.
```bash
git clone https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout b173cf28174b17f7df96c298f83c16a00d22e312
git submodule init
git submodule update
cd components
git clone --recurse-submodules git@github.com:yetifrisstlama/libesphttpd.git
export IDF_PATH=<insert_path>/esp-idf
```

Clone Espirgbani, build and flash it ...

```bash
git clone git@github.com:yetifrisstlama/Espirgbani.git
cd Espirgbani/src
make
make flash     # program esp32 over serial port, hold the FLASH button!
make web-flash # program esp32 over wifi, set hostname in Makefile
```

## fast-seek
For a smoother animation playback, the fast-seek option needs to be enabled in
the fatfs library. Use the provided .patch file ...
```bash
cd esp-idf
patch -p1 -i ../Espirgbani/src/0001-Enable-FASTSEEK-option-in-FatFs.patch
```

# SD card instructions
Format as FAT32, then copy the following files:
  * `./settings.json`
  * `./runDmd.img`
  * `./fnt/{d}.fnt`
  * `./fnt/{d}_0.bmp`

## example `settings.json`
```json
{
    "panel": {
        "test_pattern": false,
        "tp_brightness":  10,
        "is_clk_inverted": true,
        "column_swap": false,
        "latch_offset": 0,
        "extra_blank": 1,
        "clkm_div_num": 4
    },
    "power": {
        "hi": {
            "h": 9,
            "m": 0,
            "p": 50,
            "pingIp": "192.168.1.208"
        },
        "lo": {
            "h": 22,
            "m": 45,
            "p": 1
        }
    },
    "delays": {
        "font":  60,
        "color": 10,
        "ani":   15,
        "ping":  60
    },
    "log_level": "W",
    "timezone": "PST8PDT,M3.2.0,M11.1.0",
    "hostname": "espirgbani",
    "wifis": {
        "<wifi-ssid>": {
            "pw": "<wifi-password>"
        }
    }
}
```
__the `panel` section__
Not all LED panels are the same. Here the timing parameters of the I2S panel driver can be configured.

  * `test_pattern`: if true, enters a LED panel test mode instead of normal operation
  * `tp_brightness`: brightness of the test pattern, from 1 to 127. Current draw gets ridiculous for the higher values
  * `is_clk_inverted`: if `false`, data changes on the rising clock edge. If `true`, data is stable on the rising clock edge (most panels need `true`)
  * `column_swap`: if true, swap each pair of vertical columns
  * when `latch_offset` = 0: latch row-data with last pixel. For positive / negative numbers the latching happens n clock cycles earlier / later. Shifts the image horizontally
  * `extra_blank` adds N additional delay cycles after latching before enabling the LEDs to prevent ghosting artifacts from one row to another
  * `clkm_div_num` sets the I2S clock divider from 1 to 128. Set it too high and get flicker,  too low get ghost pixels. `4` (= 8 MHz pixel clock) is a good compromise for my panels

__the `power` section__
controls the display brightness depending on current time.

  * Switches to brightness `90` at `9:00`
  * But only if `192.168.1.208` responds to pings
  * Switches to brightness `1` at `22:45`

__the `delays` section__
controls delays between random animations, color and font changes.

  * Changes the clock font every 60 minutes
  * Changes color of the clock font every 10 minutes
  * Plays a new pinball animation every 15 seconds
  * tries to ping `pingIp` every 60 seconds

__log_level__ is the logging level for the serial port and the web-console on startup. Can be one of the following letters: `E = ERROR`, `W = WARN`, `I = INFO`, `D = DEBUG`, `V = VERBOSE`

__timezone__ is the local timezone in [TZ format](https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html). Look it up [here](https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv).

Note that once running and connected to the wifi, the settings.json file can be edited through the web interface.

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

## Pre-built binaries (TLDR!)
Prebuilt firmware binaries for the ESP32 and the files for the (FAT32) SD card are [here](https://github.com/yetifrisstlama/Espirgbani/releases/tag/v1.0).

# TODO
  * rewrite .json settings
