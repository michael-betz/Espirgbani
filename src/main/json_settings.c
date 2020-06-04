#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include "cJSON.h"

static const char *default_settings = "\n\
{\n\
    \"panel\": {\n\
        \"test_pattern\": 1,\n\
        \"tp_brightness\":  8,\n\
        \"is_clk_inverted\": 1,\n\
        \"column_swap\": 0,\n\
        \"latch_offset\": 0,\n\
        \"extra_blank\": 1,\n\
        \"clkm_div_num\": 16\n\
    },\n\
    \"power\": {\n\
        \"hi\": {\n\
            \"h\": 9,\n\
            \"m\": 0,\n\
            \"p\": 90,\n\
            \"pingIp\": \"192.168.1.208\"\n\
        },\n\
        \"lo\": {\n\
            \"h\": 22,\n\
            \"m\": 45,\n\
            \"p\": 1\n\
        }\n\
    },\n\
    \"delays\": {\n\
        \"font\":  60,\n\
        \"color\": 10,\n\
        \"ani\":   15,\n\
        \"ping\":  60\n\
    },\n\
    \"timezone\": \"PST8PDT,M3.2.0,M11.1.0\",\n\
    \"hostname\": \"espirgbani\",\n\
    \"wifis\": {\n\
        \"<wifi-ssid>\": {\n\
            \"pw\": \"<wifi-password>\"\n\
        }\n\
    }\n\
}\n\
"

char *readFileDyn( const char* fName, int *fSize ){
    // opens the file fName and returns it as dynamically allocated char*
    // if fSize is not NULL, copies the number of bytes read there
    // dont forget to fall free() on the char* result
    FILE *f = fopen(fName, "rb");
    if (!f) {
        ESP_LOGE( T, " fopen(%s, rb) failed: %s", fName, strerror(errno));
        return f;
    }
    fseek(f, 0, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  //same as rewind(f);
    // ESP_LOGI(T, "filesize: %d", fsize );
    char *string = malloc(fsize + 1);
    if (!string) {
        ESP_LOGE(T, "malloc(%d) failed: %s", fsize + 1, strerror(errno));
        assert(false);
    }
    fread(string, fsize, 1, f);
    fclose(f);
    string[fsize] = 0;
    if (fSize)
        *fSize = fsize;
    return string;
}

cJSON *readJsonDyn(const char* fName){
    // opens the json file `fName` and returns it as cJSON*
    // don't forget to call cJSON_Delete() on it
    cJSON *root;

    // try to read settings.json from SD card
    char *txtData = readFileDyn(fName, NULL);

    // if it does not exist, try to create it with the default settings
    if (!txtData) {
        ESP_LOGW(T, "writing default-settings file to SD card");
        FILE *f = fopen(fName, "wb");
        if (f) {
            f.write(default_settings);
            f.close();
        } else {
            ESP_LOGE(T, " fopen(%s, wb) failed: %s", fName, strerror(errno));
        }
        ESP_LOGW(T, "loading default settings");
        txtData = malloc(sizeof(default_settings));
        if (!txtData) {
            ESP_LOGE(T, "malloc(%d) failed: %s", sizeof(default_settings), strerror(errno));
            assert(false);
        }
        memcpy(txtData, default_settings, sizeof(default_settings));
    }

    // load txtData as .json
    if (!(root = cJSON_Parse(txtData))) {
        ESP_LOGE(T,"JSON Error in %s", fName);
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            ESP_LOGE(T, "Error before: %s", error_ptr);
        }
    }
    free(txtData);
    return root;
}

cJSON *g_settings = NULL;
cJSON *getSettings() {
    if (!g_settings) {
        g_settings = readJsonDyn(SETTINGS_FILE);
        if(g_settings) {
            ESP_LOGI(T, "Loaded %s:", SETTINGS_FILE);
        } else {
            ESP_LOGE(T, "Error loading %s", SETTINGS_FILE);
        }
    }
    return g_settings;
}

// get string or default - value on error
const char *jGetSD(const cJSON *j, const char *sName, const char *sDefault) {
    const cJSON *jTemp = cJSON_GetObjectItemCaseSensitive( j, sName );
    if( !cJSON_IsString(jTemp) ){
        ESP_LOGE(T, "json error: %s is not a string", sName);
        return sDefault;
    }
    return jTemp->valuestring;
}

void reloadSettings(){
    if (g_settings) {
        cJSON_Delete(g_settings);
        g_settings = NULL;
    }
}

int jGetI(json, name, default_val) {
    const cJSON *obj = cJSON_GetObjectItemCaseSensitive(json, name);
    if (!cJSON_IsNumber(obj)) {
        ESP_LOGE(T, "json error: %s is not a number, falling back to %d", name, default_val);
        return default_val;
    }
    return obj->valueint;
}
