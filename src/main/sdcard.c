#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "sdcard.h"

static const char *T = "SD_CARD";

void initSd(){
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
    slot_config.gpio_miso = GPIO_SD_MISO;
    slot_config.gpio_mosi = GPIO_SD_MOSI;
    slot_config.gpio_sck  = GPIO_SD_CLK;
    slot_config.gpio_cs   = GPIO_SD_CS;
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5	//Max number of open files
    };
    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sd", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(T, "Failed to mount SD card filesystem. ");
        } else {
            ESP_LOGE(T, "Failed to initialize the SD card (%d). ", ret);
        }
        return;
    }

    sdspi_host_set_card_clk( VSPI_HOST, 40000 );

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
}
