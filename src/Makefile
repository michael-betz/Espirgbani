#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := espirgbani

include $(IDF_PATH)/make/project.mk

spiffs-flash: ./build/spiffs.img
	$(IDF_PATH)/components/esptool_py/esptool/esptool.py write_flash -z 0x190000 $^

./build/spiffs.img: ./spiffs_data
	/home/michael/esp32/mkspiffs/mkspiffs -c $^ -b 4096 -p 256 -s 0x270000 $@

web-flash: $(APP_BIN)
	@echo "Flashing app to http POST ..."
	curl -X POST --data-binary @$(APP_BIN) http://espirgbani/flash/upload; curl http://espirgbani/reboot

.PHONY: spiffs.img, flashSpiffs