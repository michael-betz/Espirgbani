#!/bin/bash
echo "Flashing binaries to serial port "/dev/ttyUSB0" (app at offset 0x10000)..."
python2 esptool.py --chip esp32 --port "/dev/ttyUSB0" --baud 921600 --before "default_reset" --after "hard_reset" write_flash -z --flash_mode "dio" --flash_freq "40m" --flash_size detect 0xd000 ota_data_initial.bin 0x1000 bootloader.bin 0x10000 espirgbani.bin 0x8000 partitions.bin
